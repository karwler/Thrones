#include "shaders.h"
#include "server/server.h"
#include "utils/settings.h"
#include "utils/text.h"
#include <random>

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
	checkStatus(program, GL_LINK_STATUS, glGetProgramiv, glGetProgramInfoLog, name + " prog"s);
	glUseProgram(program);
}

GLuint Shader::loadShader(const string& source, GLenum type, const char* name) {
	const char* src = source.data();
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);
	checkStatus(shader, GL_COMPILE_STATUS, glGetShaderiv, glGetShaderInfoLog, name + (type == GL_VERTEX_SHADER ? " vert"s : type == GL_FRAGMENT_SHADER ? " frag"s : ""s));
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
	noiseScale(glGetUniformLocation(program, "noiseScale"))
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
	glUniform3fv(glGetUniformLocation(program, "samples"), sampleSize, reinterpret_cast<float*>(kernel));

	vec3 noise[noiseSize];
	for (uint i = 0; i < noiseSize; ++i)
		noise[i] = glm::normalize(vec3(realDist(generator) * 2.f - 1.f, realDist(generator) * 2.f - 1.f, 0.f));

	glActiveTexture(noiseTexa);
	glGenTextures(1, &texNoise);
	glBindTexture(GL_TEXTURE_2D, texNoise);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, noise);
}

ShaderBlur::ShaderBlur(const string& srcVert, const string& srcFrag, GLenum colorMap) :
	Shader(srcVert, srcFrag, "Shader blur")
{
	glUniform1i(glGetUniformLocation(program, "colorMap"), colorMap - GL_TEXTURE0);
}

ShaderSsr::ShaderSsr(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader SSR"),
	proj(glGetUniformLocation(program, "proj"))
{
	glUniform1i(glGetUniformLocation(program, "vposMap"), vposTexa - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "normMap"), normTexa - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "matlMap"), matlTexa - GL_TEXTURE0);
}

ShaderSsrColor::ShaderSsrColor(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader SSR color")
{
	glUniform1i(glGetUniformLocation(program, "colorMap"), sceneTexa - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "ssrMap"), ssr0Texa - GL_TEXTURE0);
}

ShaderLight::ShaderLight(const string& srcVert, const string& srcFrag, const Settings* sets) :
	Shader(srcVert, srcFrag, "Shader light"),
	optShadow(glGetUniformLocation(program, "optShadow")),
	optSsao(glGetUniformLocation(program, "optSsao")),
	pview(glGetUniformLocation(program, "pview")),
	viewPos(glGetUniformLocation(program, "viewPos")),
	farPlane(glGetUniformLocation(program, "farPlane")),
	lightPos(glGetUniformLocation(program, "lightPos")),
	lightAmbient(glGetUniformLocation(program, "lightAmbient")),
	lightDiffuse(glGetUniformLocation(program, "lightDiffuse")),
	lightLinear(glGetUniformLocation(program, "lightLinear")),
	lightQuadratic(glGetUniformLocation(program, "lightQuadratic"))
{
	glUniform1i(optShadow, sets->getShadowOpt());
	glUniform1i(optSsao, sets->ssao);
	glUniform1i(glGetUniformLocation(program, "colorMap"), colorTexa - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "normaMap"), normalTexa - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "depthMap"), depthTexa - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "ssaoMap"), ssao1Texa - GL_TEXTURE0);
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
	Shader(srcVert, srcFrag, "Shader final"),
	optFxaa(glGetUniformLocation(program, "optFxaa")),
	optSsr(glGetUniformLocation(program, "optSsr")),
	optBloom(glGetUniformLocation(program, "optBloom")),
	gamma(glGetUniformLocation(program, "gamma"))
{
	glUniform1i(optFxaa, sets->antiAliasing == Settings::AntiAliasing::fxaa);
	glUniform1i(optSsr, sets->ssr);
	glUniform1i(optBloom, sets->bloom);
	glUniform1i(glGetUniformLocation(program, "sceneMap"), sceneTexa - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "bloomMap"), gaussTexa - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "ssrCleanMap"), ssr1Texa - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "ssrBlurMap"), ssr0Texa - GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "matlMap"), matlTexa - GL_TEXTURE0);
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
#ifdef OPENVR
	pviewModel(glGetUniformLocation(program, "pviewModel")),
#endif
	pview(glGetUniformLocation(program, "pview"))
{
	glUniform1i(glGetUniformLocation(program, "colorMap"), wgtTexa - GL_TEXTURE0);
}

ShaderStartup::ShaderStartup(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader startup"),
	pview(glGetUniformLocation(program, "pview")),
	rect(glGetUniformLocation(program, "rect"))
{
	glUniform1i(glGetUniformLocation(program, "colorMap"), tmpTexa - GL_TEXTURE0);
}

#ifdef OPENVR
ShaderVrController::ShaderVrController(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader VR controller"),
	matrix(glGetUniformLocation(program, "matrix"))
{}

ShaderVrModel::ShaderVrModel(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader VR model"),
	matrix(glGetUniformLocation(program, "matrix")),
	diffuse(glGetUniformLocation(program, "diffuse"))
{
	//glUniform1i(glGetUniformLocation(program, "colorMap"), stlogTexa - GL_TEXTURE0);
}

ShaderVrWindow::ShaderVrWindow(const string& srcVert, const string& srcFrag) :
	Shader(srcVert, srcFrag, "Shader VR window"),
	mytexture(glGetUniformLocation(program, "mytexture"))
{
	//glUniform1i(glGetUniformLocation(program, "colorMap"), stlogTexa - GL_TEXTURE0);
}
#endif
