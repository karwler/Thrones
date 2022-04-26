#include "shaders.h"
#include "server/server.h"
#include "utils/settings.h"
#include "utils/utils.h"
#include <random>
#include <regex>

Shader::Shader(const string& srcVert, const string& srcFrag, const char* name) {
	GLuint vert = loadShader(srcVert, GL_VERTEX_SHADER, name);
	GLuint frag = loadShader(srcFrag, GL_FRAGMENT_SHADER, name);
	program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);
	glDetachShader(program, vert);
	glDetachShader(program, frag);
	glDeleteShader(vert);
	glDeleteShader(frag);
	checkStatus(program, GL_LINK_STATUS, glGetProgramiv, glGetProgramInfoLog, name + string(" prog"));
	glUseProgram(program);
}

GLuint Shader::loadShader(const string& source, GLenum type, const char* name) {
	const char* src = source.c_str();
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);
	checkStatus(shader, GL_COMPILE_STATUS, glGetShaderiv, glGetShaderInfoLog, name + string(type == GL_VERTEX_SHADER ? " vert" : type == GL_FRAGMENT_SHADER ? " frag" : ""));
	return shader;
}

template <class C, class I>
void Shader::checkStatus(GLuint id, GLenum stat, C check, I info, const string& name) {
	int res;
	if (check(id, stat, &res); res == GL_FALSE) {
		string err;
		if (check(id, GL_INFO_LOG_LENGTH, &res); res) {
			err.resize(res);
			info(id, res, nullptr, err.data());
			err = trim(err);
		}
		err = !err.empty() ? name + ':' + linend + err : name + ": unknown error";
		logError(err);
		throw std::runtime_error(err);
	}
}

pairStr Shader::splitGlobMain(const string& src) {
	sizet pmain = src.find("main()");
	return pair(src.substr(0, pmain), src.substr(pmain));
}

ShaderGeom::ShaderGeom(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader geometry"),
	proj(glGetUniformLocation(program, "proj")),
	view(glGetUniformLocation(program, "view"))
{}

ShaderDepth::ShaderDepth(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader depth"),
	pvTrans(glGetUniformLocation(program, "pvTrans")),
	pvId(glGetUniformLocation(program, "pvId")),
	lightPos(glGetUniformLocation(program, "lightPos")),
	farPlane(glGetUniformLocation(program, "farPlane"))
{}

ShaderSsao::ShaderSsao(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader SSAO"),
	proj(glGetUniformLocation(program, "proj")),
	noiseScale(glGetUniformLocation(program, "noiseScale")),
	samples(glGetUniformLocation(program, "samples"))
{
	glUniform1i(glGetUniformLocation(program, "vposMap"), vposTexa - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "normMap"), normTexa - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "noiseMap"), noiseTexa - GL_TEXTURE0);

	std::uniform_real_distribution<float> realDist(0.f, 1.f);
	std::default_random_engine generator(Com::generateRandomSeed());
	vec3 kernel[sampleSize];
	for (int i = 0; i < sampleSize; ++i) {
		vec3 sample = glm::normalize(vec3(realDist(generator) * 2.f - 1.f, realDist(generator) * 2.f - 1.f, realDist(generator))) * realDist(generator);
		float scale = float(i) / float(sampleSize);
		kernel[i] = sample * glm::mix(0.1f, 1.f, scale * scale);
	}
	glUniform3fv(samples, sampleSize, reinterpret_cast<float*>(kernel));

	vec3 noise[noiseSize];
	for (uint i = 0; i < noiseSize; ++i)
		noise[i] = glm::normalize(vec3(realDist(generator) * 2.f - 1.f, realDist(generator) * 2.f - 1.f, 0.f));

	glActiveTexture(noiseTexa);
	glGenTextures(1, &texNoise);
	glBindTexture(GL_TEXTURE_2D, texNoise);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, noise);
	glActiveTexture(texa);
}

ShaderBlur::ShaderBlur(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader blur")
{
	glUniform1i(glGetUniformLocation(program, "colorMap"), ssaoTexa - GL_TEXTURE0);
}

ShaderLight::ShaderLight(const string& srcVert, const string& srcFrag, const Settings* sets) :
	Shader(srcVert, editSource(srcFrag, sets), "Shader light"),
	pview(glGetUniformLocation(program, "pview")),
	viewPos(glGetUniformLocation(program, "viewPos")),
	screenSize(glGetUniformLocation(program, "screenSize")),
	farPlane(glGetUniformLocation(program, "farPlane")),
	lightPos(glGetUniformLocation(program, "lightPos")),
	lightAmbient(glGetUniformLocation(program, "lightAmbient")),
	lightDiffuse(glGetUniformLocation(program, "lightDiffuse")),
	lightLinear(glGetUniformLocation(program, "lightLinear")),
	lightQuadratic(glGetUniformLocation(program, "lightQuadratic"))
{
	glUniform1i(glGetUniformLocation(program, "colorMap"), colorTexa - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "normaMap"), normalTexa - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "depthMap"), depthTexa - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "ssaoMap"), blurTexa - GL_TEXTURE0);
}

string ShaderLight::editSource(const string& src, const Settings* sets) {
	auto [out, ins] = splitGlobMain(src);
	if (sets->shadowRes)
		ins = std::regex_replace(ins, std::regex(R"r((float\s+shadow\s*=\s*)[\d\.]+(\s*;))r"), sets->softShadows ? "$1calcShadowSoft()$2" : "$1calcShadowHard()$2");
	if (sets->ssao)
		ins = std::regex_replace(ins, std::regex(R"r((float\s+ssao\s*=\s*)[\d\.]+(\s*;))r"), "$1getSsao()$2");
	return out + ins;
}

ShaderBrights::ShaderBrights(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader brights")
{
	glUniform1i(glGetUniformLocation(program, "colorMap"), sceneTexa - GL_TEXTURE0);
}

ShaderGauss::ShaderGauss(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader gauss"),
	horizontal(glGetUniformLocation(program, "horizontal"))
{
	glUniform1i(glGetUniformLocation(program, "colorMap"), gaussTexa - GL_TEXTURE0);
}

ShaderFinal::ShaderFinal(const string& srcVert, const string& srcFrag, const Settings* sets) :
	Shader(srcVert, editSource(srcFrag, sets), "Shader final"),
	gamma(glGetUniformLocation(program, "gamma"))
{
	glUniform1i(glGetUniformLocation(program, "sceneMap"), sceneTexa - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "bloomMap"), gaussTexa - GL_TEXTURE0);
}

string ShaderFinal::editSource(const string& src, const Settings* sets) {
	auto [out, ins] = splitGlobMain(src);
	if (!sets->bloom) {
		out = std::regex_replace(out, std::regex(R"r(uniform\s+sampler2D\s+bloomMap\s*;)r"), "");
		ins = std::regex_replace(ins, std::regex(R"r(vec\d\s*\()r"), "");
		ins = std::regex_replace(ins, std::regex(R"r(\.\s*[rgb]+\s*\+\s*texture\s*\(\s*bloomMap\s*,\s*fragUV\s*\)\s*\.\s*[rgb]+\s*,\s*\d+\.\d+\s*\)\s*;)r"), ";");
	}
	return out + ins;
}

ShaderSkybox::ShaderSkybox(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader skybox"),
	pview(glGetUniformLocation(program, "pview")),
	viewPos(glGetUniformLocation(program, "viewPos"))
{
	glUniform1i(glGetUniformLocation(program, "skyMap"), skyboxTexa - GL_TEXTURE0);
}

ShaderGui::ShaderGui(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader GUI"),
	pview(glGetUniformLocation(program, "pview"))
{
	glUniform1i(glGetUniformLocation(program, "colorMap"), texa - GL_TEXTURE0);
}

ShaderStartup::ShaderStartup(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader startup"),
	pview(glGetUniformLocation(program, "pview")),
	rect(glGetUniformLocation(program, "rect"))
{
	glUniform1i(glGetUniformLocation(program, "colorMap"), stlogTexa - GL_TEXTURE0);
}
