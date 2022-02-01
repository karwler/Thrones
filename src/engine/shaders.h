#pragma once

#include "oven/oven.h"

class Shader {
public:
	static constexpr GLuint vpos = 0, normal = 1, uvloc = 2, tangent = 3;
	static constexpr GLuint model0 = 4, model1 = 5, model2 = 6, model3 = 7;
	static constexpr GLuint normat0 = 8, normat1 = 9, normat2 = 10;
	static constexpr GLuint diffuse = 11, specShine = 12, texid = 13, show = 14;

	static constexpr GLenum texa = GL_TEXTURE0;			// general purpose and for widget textures
	static constexpr GLenum colorTexa = GL_TEXTURE1;	// color texture of 3D objects
	static constexpr GLenum normalTexa = GL_TEXTURE2;	// normal texture of 3D objects
	static constexpr GLenum skyboxTexa = GL_TEXTURE3;	// skybox texture
	static constexpr GLenum vposTexa = GL_TEXTURE4;		// vertex vectors output by ShaderGeom
	static constexpr GLenum normTexa = GL_TEXTURE5;		// normal vectors output by ShaderGeom
	static constexpr GLenum ssaoTexa = GL_TEXTURE6;		// SSAO map output by ShaderSsao
	static constexpr GLenum blurTexa = GL_TEXTURE7;		// SSAO blur map output by ShaderBlur
	static constexpr GLenum sceneTexa = GL_TEXTURE8;	// scene colors output by ShaderLight
	static constexpr GLenum depthTexa = GL_TEXTURE9;	// shadow map
	static constexpr GLenum noiseTexa = GL_TEXTURE10;	// noise map for SSAO

protected:
	GLuint program;

public:
	Shader(const string& srcVert, const string& srcFrag, const char* name);
	~Shader();

	operator GLuint() const;

protected:
	static pairStr splitGlobMain(const string& src);

private:
	static GLuint loadShader(const string& source, GLenum type, const char* name);
	template <class C, class I> static void checkStatus(GLuint id, GLenum stat, C check, I info, const string& name);
};

inline Shader::~Shader() {
	glDeleteProgram(program);
}

inline Shader::operator GLuint() const {
	return program;
}

class ShaderGeom : public Shader {
public:
	static constexpr char fileVert[] = "geom.vert";
	static constexpr char fileFrag[] = "geom.frag";

	GLint proj, view;

	ShaderGeom(const string& srcVert, const string& srcFrag);
};

class ShaderDepth : public Shader {
public:
	static constexpr char fileVert[] = "depth.vert";
	static constexpr char fileFrag[] = "depth.frag";

	GLint pvTrans, pvId;
	GLint lightPos, farPlane;

	ShaderDepth(const string& srcVert, const string& srcFrag);
};

class ShaderSsao : public Shader {
public:
	static constexpr char fileVert[] = "frame.vert";
	static constexpr char fileFrag[] = "ssao.frag";

	GLint proj, noiseScale, samples;
private:
	GLuint texNoise;

	static constexpr int sampleSize = 64;
	static constexpr uint noiseSize = 16;

public:
	ShaderSsao(const string& srcVert, const string& srcFrag);
	~ShaderSsao();
};

inline ShaderSsao::~ShaderSsao() {
	glDeleteTextures(1, &texNoise);
}

class ShaderBlur : public Shader {
public:
	static constexpr char fileVert[] = "frame.vert";
	static constexpr char fileFrag[] = "blur.frag";

	ShaderBlur(const string& srcVert, const string& srcFrag);
};

class ShaderLight : public Shader {
public:
	static constexpr char fileVert[] = "light.vert";
	static constexpr char fileFrag[] = "light.frag";

	GLint pview, viewPos;
	GLint screenSize, farPlane;
	GLint lightPos, lightAmbient, lightDiffuse, lightLinear, lightQuadratic;

	ShaderLight(const string& srcVert, const string& srcFrag, const Settings* sets);

private:
	static string editSource(const string& src, const Settings* sets);
};

class ShaderBrights : public Shader {
public:
	static constexpr char fileVert[] = "frame.vert";
	static constexpr char fileFrag[] = "brights.frag";

	ShaderBrights(const string& srcVert, const string& srcFrag);
};

class ShaderGauss : public Shader {
public:
	static constexpr char fileVert[] = "frame.vert";
	static constexpr char fileFrag[] = "gauss.frag";

	GLuint horizontal;

	ShaderGauss(const string& srcVert, const string& srcFrag);
};

class ShaderFinal : public Shader {
public:
	static constexpr char fileVert[] = "frame.vert";
	static constexpr char fileFrag[] = "final.frag";

	ShaderFinal(const string& srcVert, const string& srcFrag, const Settings* sets);

private:
	static string editSource(const string& src, const Settings* sets);
};

class ShaderSkybox : public Shader {
public:
	static constexpr char fileVert[] = "skybox.vert";
	static constexpr char fileFrag[] = "skybox.frag";

	GLint pview, viewPos;

	ShaderSkybox(const string& srcVert, const string& srcFrag);
};

class ShaderGui : public Shader {
public:
	static constexpr char fileVert[] = "gui.vert";
	static constexpr char fileFrag[] = "gui.frag";

	GLint pview, rect, uvrc, zloc;
	GLint color;

	ShaderGui(const string& srcVert, const string& srcFrag);
};
