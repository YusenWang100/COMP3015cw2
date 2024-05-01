#include "scenebasic_uniform.h"

#include <cmath>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include <GLFW/glfw3.h>
#include "helper/glutils.h"
#include "scene0/objloader.h"

using std::cerr;
using std::endl;
using std::string;

using glm::vec3;

void Light::Init() {
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(UBOData), &uboData, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
void Light::Update() {
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(UBOData), &uboData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
void Light::Bind(int index) { glBindBufferBase(GL_UNIFORM_BUFFER, index, ubo); }

SceneBasic_Uniform::SceneBasic_Uniform() {}
void SceneBasic_Uniform::createFBO() {
    colorTex0.Create(width, height, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE);
    colorTex0.Bind(0);

    colorTex1.Create(width, height, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE);
    colorTex1.Bind(0);
    depthTex.Create(width, height, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorTex0.texture, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, colorTex1.texture, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTex.texture, 0);

    GLenum drawbuffers[]{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, drawbuffers);

    auto a = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (a != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "failed to create fbo, %X\n", a);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //==============
    for (int i = 0; i < 2; ++i) {
        blurColorTexs[i].Create(width, height, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE);
        glGenFramebuffers(1, &blurFbos[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, blurFbos[i]);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, blurColorTexs[i].texture, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        a = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (a != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "failed to create fbo, %X\n", a);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
	float planeVertices[] = {
		// positions            // normals         // texcoords
		 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
		-25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
		-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

		 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
		-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
		 25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
	};

	// plane VAO
	unsigned int planeVBO;
	glGenVertexArrays(1, &planeVAO);
	glGenBuffers(1, &planeVBO);
	glBindVertexArray(planeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glBindVertexArray(0);
}

void SceneBasic_Uniform::deleteFBO() {
    glDeleteFramebuffers(1, &fbo);
    colorTex0.Destroy();
    colorTex1.Destroy();
    depthTex.Destroy();
    for (int i = 0; i < 2; ++i) {
        glDeleteFramebuffers(1, &blurFbos[i]);
        blurColorTexs[i].Destroy();
    }
}
void SceneBasic_Uniform::createProgram(GLSLProgram& program, size_t count, const char* const* files) {
    try {
        for (size_t i = 0; i < count; ++i) program.compileShader(files[i]);
        program.link();
        program.use();
    } catch (GLSLProgramException& e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}
void SceneBasic_Uniform::compile() {
    {
        auto vertFile = "shader/scene0/default.vert";
        auto fragFile = "shader/scene0/default.frag";
        const char* a[]{vertFile, fragFile};
        createProgram(defaultProgram, std::size(a), a);
        defaultProgram.use();
        glUniformBlockBinding(defaultProgram.getHandle(),
                              glGetUniformBlockIndex(defaultProgram.getHandle(), "UBOCamera"), 0);
        glUniformBlockBinding(defaultProgram.getHandle(),
                              glGetUniformBlockIndex(defaultProgram.getHandle(), "UBOLight"), 1);
        glUniformBlockBinding(defaultProgram.getHandle(),
                              glGetUniformBlockIndex(defaultProgram.getHandle(), "UBOTransform"), 2);
        glUniformBlockBinding(defaultProgram.getHandle(),
                              glGetUniformBlockIndex(defaultProgram.getHandle(), "UBOMaterial"), 3);
        glUniform1i(glGetUniformLocation(defaultProgram.getHandle(), "albedoMap"), 0);
        glUniform1i(glGetUniformLocation(defaultProgram.getHandle(), "normalMap"), 1);
    }
    {
        auto vertFile = "shader/scene0/skybox.vert";
        auto fragFile = "shader/scene0/skybox.frag";
        const char* a[]{vertFile, fragFile};
        createProgram(skyboxProgram, std::size(a), a);
        skyboxProgram.use();
        glUniformBlockBinding(skyboxProgram.getHandle(), glGetUniformBlockIndex(skyboxProgram.getHandle(), "UBOCamera"),
                              0);
        glUniform1i(glGetUniformLocation(skyboxProgram.getHandle(), "skybox"), 0);
    }
    {
        auto vertFile = "shader/scene0/blur.vert";
        auto fragFile = "shader/scene0/blur.frag";
        const char* a[]{vertFile, fragFile};
        createProgram(blurProgram, std::size(a), a);
        blurProgram.use();
        glUniform1i(glGetUniformLocation(blurProgram.getHandle(), "tex"), 0);
    }
    {
        auto vertFile = "shader/scene0/blur.vert";
        auto fragFile = "shader/scene0/combine.frag";
        const char* a[]{vertFile, fragFile};
        createProgram(combineProgram, std::size(a), a);
        combineProgram.use();
        glUniform1i(glGetUniformLocation(combineProgram.getHandle(), "tex"), 0);
        glUniform1i(glGetUniformLocation(combineProgram.getHandle(), "texHigh"), 1);
    }

	try {
		proDepth.compileShader("shader/scene0/shadow_mapping_depth.vert");
		proDepth.compileShader("shader/scene0/shadow_mapping_depth.frag");
		proDepth.link();
		proDepth.use();
	}
	catch (GLSLProgramException& e) {
		cerr << e.what() << endl;
		exit(EXIT_FAILURE);
	}

	glUniformBlockBinding(proDepth.getHandle(),
		glGetUniformBlockIndex(proDepth.getHandle(), "LightMatrixUBO"), 0);

	glUniformBlockBinding(proDepth.getHandle(),
		glGetUniformBlockIndex(proDepth.getHandle(), "TransformUBO"), 1);

	try {
        proPBR.compileShader("shader/scene0/pbr.vert");
        proPBR.compileShader("shader/scene0/pbr.frag");
        proPBR.link();
        proPBR.use();
	}
	catch (GLSLProgramException& e) {
		cerr << e.what() << endl;
		exit(EXIT_FAILURE);
	}

	try {
		proDebugQuad.compileShader("shader/scene0/debug_quad.vert");
		proDebugQuad.compileShader("shader/scene0/debug_quad.frag");
		proDebugQuad.link();
		proDebugQuad.use();
	}
	catch (GLSLProgramException& e) {
		cerr << e.what() << endl;
		exit(EXIT_FAILURE);
	}
}

// renders the 3D scene
// --------------------
void SceneBasic_Uniform::renderScene(GLSLProgram& shader)
{
	// floor
	glm::mat4 model = glm::mat4(1.0f);
	shader.setUniform("model", model);
	glBindVertexArray(planeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// cubes
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0));
	model = glm::scale(model, glm::vec3(0.5f));
	shader.setUniform("model", model);

	transformUBOData.transform = lightSpaceMatrix;

	glBindBuffer(GL_UNIFORM_BUFFER, transformUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(TransformUBO), &transformUBOData);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, transformUBO);

	renderCube();
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(2.0f, 0.0f, 1.0));
	model = glm::scale(model, glm::vec3(0.5f));
	shader.setUniform("model", model);
	renderCube();
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-1.0f, 0.0f, 2.0));
	model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
	model = glm::scale(model, glm::vec3(0.25));
	shader.setUniform("model", model);
	renderCube();
}

void SceneBasic_Uniform::createDepthFBO()
{
	// configure depth map FBO
// -----------------------
	glGenFramebuffers(1, &depthMapFBO);
	// create depth texture
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Frame buffer is not complete.\n";
	}
}

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void SceneBasic_Uniform::renderCube()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			 // bottom face
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			  1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 // top face
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			  1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			  1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			  1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void SceneBasic_Uniform::renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void SceneBasic_Uniform::initScene() {
    compile();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0, 0, 0, 0);
    // create fbo
    //  config camera
    camera.Position = {0, 30, 100};
    camera.fovy = 45.f;
    camera.zNear = 0.1;
    camera.zFar = 500;
    camera.Init();

    // config lights
    light.uboData.lightPosition = {-20.0f, 20.0f, 20.0f, 1};
    light.uboData.movingLightPosition = {3.0f, 5.0f, 15.0f, 1};
    light.uboData.movingLightColor = {1.0f, 0.0f, 0.0f, 1};
    light.uboData.ambientColor = {0.1f, 0.1f, 0.1f, 1};
    light.uboData.lightColor = {1.0f, 1.0f, 1.0f, 1};
    light.uboData.directionalLightColor = {1.0f, 1.0f, 1.0f, 1};

    light.uboData.spotLightPosition0 = {-23.0f, 2.0f, 27.5f};
    light.uboData.spotLightDirection0 = glm::normalize(glm::vec3(0.f, -0.0f, 1.0f));
    light.uboData.spotLightPosition1 = {-17.0f, 2.0f, 27.5f};
    light.uboData.spotLightDirection1 = glm::normalize(glm::vec3(0.f, -0.0f, 1.0f));
    light.uboData.spotLightColor = {1.0f, 1.0f, 1.0f};
    light.uboData.spotLightPenumbra = glm::radians(35.f);
    light.uboData.spotLightUmbra = glm::radians(45.f);
    light.Init();

    // skybox
    skybox.faces = {
        "./media/textures/skybox/CloudyCrown_Midday_Right.png", "./media/textures/skybox/CloudyCrown_Midday_Left.png",
        "./media/textures/skybox/CloudyCrown_Midday_Up.png",    "./media/textures/skybox/CloudyCrown_Midday_Down.png",
        "./media/textures/skybox/CloudyCrown_Midday_Front.png", "./media/textures/skybox/CloudyCrown_Midday_Back.png"};
    skybox.Init();

    // config scenes
    loadModelObj(house, "media/models/house.obj");
    loadModelObj(ground, "media/models/Plane.obj");
    loadModelObj(tree, "media/models/tree.obj");
    loadModelObj(trunk, "media/models/trunk.obj");
    loadModelObj(table, "media/models/table.obj");
    loadModelObj(dragon, "media/models/dragon.obj");
    loadModelObj(crate, "media/models/cube.obj");
    loadModelObj(dog, "media/models/dog/12228_Dog_v1_L2.obj");
    loadModelObj(wooden, "media/models/wooden/wooden.obj");
    loadModelObj(plants, "media/models/plants/plants.obj");
    loadModelObj(signature, "media/models/signature.obj");
    loadModelObj(sphere, "media/models/sphere.obj");
    loadModelObj(car, "media/models/toycar/car.obj");

    car.transform.translation = glm::vec3(-20., 0, 20);
    car.transform.scale = glm::vec3(4.);
    // car.transform.rotation = glm::rotate(glm::quat(1, 0, 0, 0), glm::radians(-90.f), glm::vec3(0, 1, 0));
    car.transform.Update();

    house.transform.scale = glm::vec3(4.);
    house.transform.rotation = glm::rotate(glm::quat(1, 0, 0, 0), glm::radians(-90.f), glm::vec3(0, 1, 0));
    house.transform.Update();

    ground.transform.scale = glm::vec3(4.);
    ground.transform.rotation = glm::rotate(glm::quat(1, 0, 0, 0), glm::radians(-90.f), glm::vec3(0, 1, 0));
    ground.transform.Update();

    crate.transform.translation = {-5.0f, 1.0f, 20.0f};
    crate.transform.Update();

    wooden.transform.translation = {-5.0f, 1.0f, 20.0f};
    wooden.transform.Update();

    plants.transform.translation = {0.0f, 0.0f, 5.0f};
    plants.transform.scale = {0.01f, 0.01f, 0.01f};
    plants.transform.Update();

    signature.transform.translation = {0.0f, 10.0f, 10.0f};
    signature.transform.rotation = glm::rotate(glm::quat(1, 0, 0, 0), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    signature.transform.scale = {5.0f, 5.0f, 5.0f};
    signature.transform.Update();

    //
    triangle.positions = {
        {-1, -1, 0},
        {3, -1, 0},
        {-1, 3, 0},
    };
    triangle.texcoords = {
        {0, 0},
        {2, 0},
        {0, 2},
    };
    auto& primitive = triangle.primitives.emplace_back();
    primitive.topology = GL_TRIANGLES;
    primitive.vertexCount = 3;
    triangle.Init();

	glGenBuffers(1, &lightMatrixUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, lightMatrixUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(TransformUBO), &lightMatrixUBOData, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glGenBuffers(1, &transformUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, transformUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(TransformUBO), &transformUBOData, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

    createDepthFBO();
}

void SceneBasic_Uniform::update(float t, GLFWwindow* window) {
    auto deltaTime = t / 1000.;
    tSum += t;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(LEFT, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        camera.ProcessKeyboard(UP, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        camera.ProcessKeyboard(DOWN, deltaTime);
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera.SetMaxSpeed();
    } else if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        camera.SetMinSpeed();
    } else {
        camera.SetNormalSpeed();
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        dogOffset += 0.02f;
        isMoving = true;
    }

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        dogOffset -= 0.02f;
        isMoving = true;
    }
    if (tailWiggleAngle > 8 || tailWiggleAngle < -8) {
        tailWiggleDirectionLeft = !tailWiggleDirectionLeft;
    }
    if (tailWiggleDirectionLeft) {
        tailWiggleAngle += 1.7f;
    } else {
        tailWiggleAngle -= 1.7f;
    }
    if (isMoving) {
        if (legsAngle > 20 || legsAngle < -20) {
            legsMovementDirectionForward = !legsMovementDirectionForward;
        }
        if (legsMovementDirectionForward) {
            legsAngle += 6.0;
        } else {
            legsAngle -= 6.0;
        }
        isMoving = false;
    }

    // update
    treeRotation += 0.1f * treeRotationDirection;

    if (treeRotation > 5.0f || treeRotation < -5.0f) {
        treeRotationDirection = -treeRotationDirection;
    }

    // update light
    light.uboData.spotLightColor = glm::vec3(std::sin(tSum / 1000.) * 0.5 + 0.5);
    light.Update();
}

void SceneBasic_Uniform::drawTree(glm::vec3 const& pos, glm::vec3 const& scale) {
    tree.transform.translation = pos;
    tree.transform.rotation =
        glm::rotate(glm::quat(1, 0, 0, 0), glm::radians(treeRotation), glm::vec3(0.0f, 0.0f, 1.0f));
    tree.transform.scale = scale;
    tree.transform.Update();
    tree.Draw();

    trunk.transform.translation = pos;
    trunk.transform.scale = scale;
    trunk.transform.Update();
    trunk.Draw();
}
void SceneBasic_Uniform::drawTable(glm::vec3 const& pos, glm::vec3 const& scale) {
    table.transform.translation = pos;
    table.transform.scale = scale;
    table.transform.rotation = glm::rotate(glm::quat(1, 0, 0, 0), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    table.transform.Update();
    table.Draw();
}
void SceneBasic_Uniform::drawDog() {
    sphere.transform.translation = {0.0f, 1.5f, dogOffset};
    sphere.transform.scale = {0.6f, 0.6f, 1.2f};
    sphere.transform.Update();
    sphere.Draw();

    // legs
    sphere.transform.translation = {-0.3f, 0.75f, -0.6f + dogOffset};
    sphere.transform.scale = {0.5f * 0.3f, 2.0f * 0.3f, 0.5f * 0.3f};
    sphere.transform.rotation = glm::rotate(glm::quat(1, 0, 0, 0), glm::radians(legsAngle), {1.0f, 0.0f, 0.0f});
    sphere.transform.Update();
    sphere.Draw();

    sphere.transform.translation = {0.3f, -2.5f * 0.3f + 1.5f, -0.6f + dogOffset};
    sphere.transform.scale = {0.5f * 0.3f, 0.6f, 0.5f * 0.3f};
    sphere.transform.rotation = glm::rotate(glm::quat(1, 0, 0, 0), glm::radians(-legsAngle), {1.0f, 0.0f, 0.0f});
    sphere.transform.Update();
    sphere.Draw();

    sphere.transform.translation = {0.3f, -2.5f * 0.3f + 1.5f, 2.0 * 0.3f + dogOffset};
    sphere.transform.scale = {0.5f * 0.3f, 0.6f, 0.5f * 0.3f};
    sphere.transform.rotation = glm::rotate(glm::quat(1, 0, 0, 0), glm::radians(legsAngle), {1.0f, 0.0f, 0.0f});
    sphere.transform.Update();
    sphere.Draw();

    sphere.transform.translation = {-0.3f, -2.5f * 0.3f + 1.5f, 0.6 + dogOffset};
    sphere.transform.scale = {0.5f * 0.3f, 0.6f, 0.5f * 0.3f};
    sphere.transform.rotation = glm::rotate(glm::quat(1, 0, 0, 0), glm::radians(-legsAngle), {1.0f, 0.0f, 0.0f});
    sphere.transform.Update();
    sphere.Draw();

    // tail
    sphere.transform.translation = {0.0f, 0.0f + 1.5f, -3.8f * 0.3f + dogOffset};
    sphere.transform.rotation = glm::rotate(glm::quat(1, 0, 0, 0), glm::radians(-30.0f), {1.0f, 0.0f, 0.0f});
    sphere.transform.rotation =
        glm::rotate(sphere.transform.rotation, glm::radians(tailVerticalAngle), {1.0f, 0.0f, 0.0f});
    sphere.transform.rotation =
        glm::rotate(sphere.transform.rotation, glm::radians(tailHorizontalAngle), {0.0f, 1.0f, 0.0f});
    sphere.transform.rotation =
        glm::rotate(sphere.transform.rotation, glm::radians(tailWiggleAngle), {0.0f, 1.0f, 0.0f});
    sphere.transform.scale = {0.5f * 0.3f, 0.5f * 0.3f, 1.8f * 0.3f};
    sphere.transform.Update();
    sphere.Draw();

    // Head
    sphere.transform.rotation = glm::quat(1, 0, 0, 0);
    sphere.transform.translation = {0.0f, 2.5f * 0.3f + 1.5f, 3.0f * 0.3f + dogOffset};
    sphere.transform.scale = {1.5f * 0.3f, 1.55f * 0.3f, 1.6f * 0.3f};
    sphere.transform.Update();
    sphere.Draw();

    // Nose
    sphere.transform.translation = {0.0f, 2.2f * 0.3f + 1.5f, 4.2f * 0.3f + dogOffset};
    sphere.transform.scale = {0.8f * 0.3f, 0.5f * 0.3f, 1.5f * 0.3f};
    sphere.transform.Update();
    sphere.Draw();

    // Ear
    sphere.transform.translation = {-0.8f * 0.3f, 3.8f * 0.3f + 1.5f, 2.6f * 0.3f + dogOffset};
    sphere.transform.scale = {0.5f * 0.3f, 0.3f, 0.5f * 0.3f};
    sphere.transform.Update();
    sphere.Draw();

    sphere.transform.translation = {0.8f * 0.3f, 3.8f * 0.3f + 1.5f, 2.6f * 0.3f + dogOffset};
    sphere.transform.scale = {0.5f * 0.3f, 0.3f, 0.5f * 0.3f};
    sphere.transform.Update();
    sphere.Draw();

    sphere.transform.translation = {0.5f * 0.3f, 3.0f * 0.3f + 1.5f, 4.4f * 0.3f + dogOffset};
    sphere.transform.scale = {0.25f * 0.3f, 0.25f * 0.3f, 0.25f * 0.3f};
    sphere.transform.Update();
    sphere.Draw();

    sphere.transform.translation = {-0.5f * 0.3f, 3.0f * 0.3f + 1.5f, 4.4f * 0.3f + dogOffset};
    sphere.transform.scale = {0.25f * 0.3f, 0.25f * 0.3f, 0.25f * 0.3f};
    sphere.transform.Update();
    sphere.Draw();
}
void SceneBasic_Uniform::drawDragon() {
    dragon.transform.translation = {-2.0f, 2.5f, 15.0f};
    dragon.transform.scale = {0.5f, 0.5f, 0.5f};
    dragon.transform.Update();
    dragon.Draw();

    dragon.transform.translation = {1.0f, 2.5f, 15.0f};
    dragon.transform.scale = {0.5f, 0.5f, 0.5f};
    dragon.transform.Update();
    dragon.Draw();
}
void SceneBasic_Uniform::drawCrate() {
    crate.Draw();
    wooden.Draw();
}

void SceneBasic_Uniform::depthPass()
{
	glClearColor(0.4f, 0.6f, 0.9f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 1. render depth of scene to texture (from light's perspective)
	// --------------------------------------------------------------
	glm::mat4 lightProjection, lightView;
	float near_plane = 1.0f, far_plane = 7.5f;
	lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
	lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
	lightSpaceMatrix = lightProjection * lightView;
	// render scene from light's point of view
	proDepth.use();
	proDepth.setUniform("lightSpaceMatrix", lightSpaceMatrix);

    lightMatrixUBOData.transform = lightSpaceMatrix;

	glBindBuffer(GL_UNIFORM_BUFFER, lightMatrixUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(TransformUBO), &lightMatrixUBOData);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, lightMatrixUBO);

	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE0);

	renderScene(proDepth);

    //drawScene();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// reset viewport
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// render Depth map to quad for visual debugging
	// ---------------------------------------------
	proDebugQuad.use();
	proDebugQuad.setUniform("near_plane", near_plane);
	proDebugQuad.setUniform("far_plane", far_plane);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	renderQuad();
}

void SceneBasic_Uniform::drawScene()
{
	// draw scene
	ground.Draw();

	car.Draw();

	drawTree({ -20.0f, 0.0f, -10.0f }, { 2.0f, 2.0f, 2.0f });
	drawTree({ 20.0f, 0.0f, -10.0f }, { 2.0f, 2.0f, 2.0f });
	drawTree({ -20.0f, 0.0f, 10.0f }, { 2.0f, 2.0f, 2.0f });
	drawTree({ 20.0f, 0.0f, 10.0f }, { 2.0f, 2.0f, 2.0f });

	drawTable({ 0.0f, 2.5f, 15.0f }, { 1.0f, 1.0f, 1.0f });

	drawDog();
	drawDragon();
	drawCrate();

	plants.Draw();
	// glDisable(GL_BLEND);
	signature.Draw();
}

void SceneBasic_Uniform::render() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // camera
    defaultProgram.use();
    camera.Bind(0);
    light.Bind(1);

    // draw scene
    house.Draw();

    ground.Draw();

    car.Draw();

    drawTree({-20.0f, 0.0f, -10.0f}, {2.0f, 2.0f, 2.0f});
    drawTree({20.0f, 0.0f, -10.0f}, {2.0f, 2.0f, 2.0f});
    drawTree({-20.0f, 0.0f, 10.0f}, {2.0f, 2.0f, 2.0f});
    drawTree({20.0f, 0.0f, 10.0f}, {2.0f, 2.0f, 2.0f});

    drawTable({0.0f, 2.5f, 15.0f}, {1.0f, 1.0f, 1.0f});

    drawDog();
    drawDragon();
    drawCrate();

    plants.Draw();
    // glDisable(GL_BLEND);
    signature.Draw();

    //
    skyboxProgram.use();
    skybox.Draw();

    bool flag = true;
    bool horizontal = true;
    blurProgram.use();
    auto loc = glGetUniformLocation(blurProgram.getHandle(), "horizontal");
    for (int i = 0; i < 20; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, blurFbos[horizontal]);
        glClear(GL_COLOR_BUFFER_BIT);
        glUniform1i(loc, horizontal);
        if (flag)
            colorTex1.Bind(0);
        else
            blurColorTexs[!horizontal].Bind(0);
        triangle.Draw();
        horizontal = !horizontal;
        if (flag) flag = false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    combineProgram.use();
    colorTex0.Bind(0);
    blurColorTexs[!horizontal].Bind(1);
    triangle.Draw();

    //depthPass();
}
void SceneBasic_Uniform::resize(int w, int h) {
    if (w == 0 || h == 0) return;
    width = w;
    height = h;
    deleteFBO();
    createFBO();
    glViewport(0, 0, w, h);
    camera.aspect = float(w) / h;
    camera.Update();
}
void SceneBasic_Uniform::mouseMoveCallback(double xpos, double ypos) {
    auto xOffset = static_cast<float>(xpos - lastMousePosition.x);
    auto yOffset = static_cast<float>(lastMousePosition.y - ypos);

    if (firstEnter) {
        lastMousePosition.x = static_cast<float>(xpos);
        lastMousePosition.y = static_cast<float>(ypos);
        firstEnter = false;
    }

    lastMousePosition.x = static_cast<float>(xpos);
    lastMousePosition.y = static_cast<float>(ypos);

    if (rightMouseButtonDown) {
        camera.ProcessMouseMovement(xOffset, yOffset);
    }
}
void SceneBasic_Uniform::mouseButtonCallback(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_2 && action == GLFW_PRESS) {
        rightMouseButtonDown = true;
    }

    if (button == GLFW_MOUSE_BUTTON_2 && action == GLFW_RELEASE) {
        rightMouseButtonDown = false;
    }
}