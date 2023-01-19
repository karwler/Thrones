#pragma once

#include "oven/oven.h"

class Shader {
public:
	static constexpr GLuint vpos = 0, normal = 1, uvloc = 2, tangent = 3;
	static constexpr GLuint model0 = 4, model1 = 5, model2 = 6, model3 = 7;
	static constexpr GLuint normat0 = 8, normat1 = 9, normat2 = 10;
	static constexpr GLuint diffuse = 11, texid = 12, show = 13;

	static constexpr GLenum tmpTexa = GL_TEXTURE0;		// for temporary or intermediate textures
	static constexpr GLenum colorTexa = GL_TEXTURE1;	// color texture of 3D objects for ShaderLight
	static constexpr GLenum normalTexa = GL_TEXTURE2;	// normal texture of 3D objects for ShaderLight
	static constexpr GLenum skyboxTexa = GL_TEXTURE3;	// skybox texture for ShaderSkybox
	static constexpr GLenum vposTexa = GL_TEXTURE4;		// vertex vectors output by ShaderGeom
	static constexpr GLenum normTexa = GL_TEXTURE5;		// normal vectors output by ShaderGeom
	static constexpr GLenum matlTexa = GL_TEXTURE6;		// material properties map output by ShaderGeom
	static constexpr GLenum ssao0Texa = GL_TEXTURE7;	// SSAO map output by ShaderSsao
	static constexpr GLenum ssao1Texa = GL_TEXTURE8;	// SSAO blur map output by ShaderBlur
	static constexpr GLenum sceneTexa = GL_TEXTURE9;	// scene colors output by ShaderLight
	static constexpr GLenum gaussTexa = GL_TEXTURE10;	// blurred bright colors in scene by ShaderBrights and ShaderGauss for ShaderFinal
	static constexpr GLenum ssr0Texa = GL_TEXTURE11;	// reflection map output by ShaderSsr and ShaderBlur
	static constexpr GLenum ssr1Texa = GL_TEXTURE12;	// reflection map output by ShaderSsrColor
	static constexpr GLenum shadowTexa = GL_TEXTURE13;	// shadow map by ShaderDepth
	static constexpr GLenum noiseTexa = GL_TEXTURE14;	// noise map for ShaderSsao
	static constexpr GLenum wgtTexa = GL_TEXTURE15;		// widget textures for ShderGui

protected:
	GLuint program;

public:
	Shader(const string& srcVert, const string& srcFrag, const char* name);
	~Shader();

	operator GLuint() const;

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

	GLint proj, noiseScale;
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
	static constexpr char fileFragM[] = "blurMono.frag";
	static constexpr char fileFragC[] = "blurColor.frag";

	ShaderBlur(const string& srcVert, const string& srcFrag, GLenum colorMap);
};

class ShaderSsr : public Shader {
public:
	static constexpr char fileVert[] = "frame.vert";
	static constexpr char fileFrag[] = "ssr.frag";

	GLint proj;

	ShaderSsr(const string& srcVert, const string& srcFrag);

	void setMaterials(const vector<Material>& matls) const;
};

class ShaderSsrColor : public Shader {
public:
	static constexpr char fileVert[] = "frame.vert";
	static constexpr char fileFrag[] = "ssrColor.frag";

	ShaderSsrColor(const string& srcVert, const string& srcFrag);
};

class ShaderLight : public Shader {
public:
	static constexpr char fileVert[] = "light.vert";
	static constexpr char fileFrag[] = "light.frag";

	GLint optShadow, optSsao;
	GLint pview, viewPos;
	GLint farPlane, lightPos, lightAmbient, lightDiffuse, lightLinear, lightQuadratic;

	ShaderLight(const string& srcVert, const string& srcFrag, const Settings* sets);

	void setMaterials(const vector<Material>& matls) const;
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

	GLint optFxaa, optSsr, optBloom;
	GLint gamma;

	ShaderFinal(const string& srcVert, const string& srcFrag, const Settings* sets);

	void setMaterials(const vector<Material>& matls) const;
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
	static constexpr char fileVertVr[] = "vrGui.vert";
	static constexpr char fileFrag[] = "gui.frag";

	static constexpr GLuint rect = 0, uvrc = 1;

#ifdef OPENVR
	GLint pviewModel;
#endif
	GLint pview;

	ShaderGui(const string& srcVert, const string& srcFrag);
};

class ShaderStartup : public Shader {
public:
	static constexpr char fileVert[] = "startup.vert";
	static constexpr char fileFrag[] = "startup.frag";

	GLint pview, rect;

	ShaderStartup(const string& srcVert, const string& srcFrag);
};

#ifdef OPENVR
class ShaderVrController : public Shader {
public:
	static constexpr char fileVert[] = "vrController.vert";
	static constexpr char fileFrag[] = "vrController.frag";

	GLint matrix;

	ShaderVrController(const string& srcVert, const string& srcFrag);
};

class ShaderVrModel : public Shader {
public:
	static constexpr char fileVert[] = "vrModel.vert";
	static constexpr char fileFrag[] = "vrModel.frag";

	GLint matrix;
	GLint diffuse;

	ShaderVrModel(const string& srcVert, const string& srcFrag);
};

class ShaderVrWindow : public Shader {
public:
	static constexpr char fileVert[] = "vrWindow.vert";
	static constexpr char fileFrag[] = "vrWindow.frag";

	GLint mytexture;

	ShaderVrWindow(const string& srcVert, const string& srcFrag);
};
#endif
