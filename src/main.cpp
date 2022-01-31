#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <../include/stb_image.h>

#include <../include/shader.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
float * generateCanvas();
float * updateCanvas(float *currentCanvas, int update);
void processInput(GLFWwindow *window);

void initializeCanvas();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int main()
{
	// glfw: initialize and configure
    std::cout << "Starting..."  << std::endl;
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); //uncomment this statement to fix compilation on OS X
#endif

	// glfw window creation
    std::cout << "Creating window..."  << std::endl;
	GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Sandy", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// glad: load all OpenGL function pointers
    std::cout << "Loading OpenGL function pointers..."  << std::endl;
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

    // glEnable(GL_DEBUG_OUTPUT);

    std::cout << "Loading shaders..."  << std::endl;
    Shader canvasShader("src/shader.vs", "src/shader.fs");

    float vertices[] = {
        // positions          // colors           // texture coords
        1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
        1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
        -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
        -1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left 
    };

    unsigned int indices[] = {  
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    // create vertex buffer object (used to store vertices on GPU), vertex array object, and elemental buffer object (create rectangle from two triangles)
    // VAO is required for Core OpenGL
    std::cout << "Creating VAO, VBO, and EBO..."  << std::endl;
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO); 

    // bind VBO and EBO to buffers
    // copy canvas vertex data to the buffer (using dynamic draw as we will be updating the texture frequently)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

    // instruct OpenGL on how to interpret the vertex data 
    // 3 * sizeof(float) is the stride, may need to tweak (stride is space between  vertex attributes)
    // position attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // color attributes
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3* sizeof(float)));
    glEnableVertexAttribArray(1);

    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // unbind buffer now that glVertexAttribPointer registered VBO as the vertex attribute's bound VBO
    // glBindBuffer(GL_ARRAY_BUFFER, 0);

    float *canvasData = generateCanvas();

    std::cout << "Creating texture..."  << std::endl;
    unsigned int texture1;
    glGenTextures(1, &texture1);
    // glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_FLOAT, canvasData);
    glGenerateMipmap(GL_TEXTURE_2D);

    std::cout << "Texture initialized..."  << std::endl;

    canvasShader.use();
    glUniform1i(glGetUniformLocation(canvasShader.ID, "texture1"), 0);


    // uncomment to activate wireframe mode
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    float *canvasUpdate;
    canvasUpdate = new float[(512 * 512) * 4];
    int step = 0;

	// render loop
	while (!glfwWindowShouldClose(window))
	{
		// input
		processInput(window);

        // update texture
        // canvasUpdate = updateCanvas(canvasData, step);
        // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_FLOAT, canvasUpdate);
        // step++;

		// render
		// glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		// glClear(GL_COLOR_BUFFER_BIT);

        // bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);

        canvasShader.use();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
	// glfw: terminate, clearing all previously allocated GLFWresources.
	//---------------------------------------------------------------
	glfwTerminate();
	return 0;
}

float * generateCanvas() {
    std::cout << "Generating canvas..."  << std::endl;
    float *canvasData;
    canvasData = new float[(512 * 512) * 4];
    int value = 0;
    int i = 0;
    for(int row = 0; row < 512; row++) {
        for(int col = 0; col < 512; col++) {
            // pattern
            // canvasData[i] = (float)((value%255)/255.0);
            // canvasData[i + 1] = (float)((value%255)/255.0);
            // canvasData[i + 2] = (float)((value%255)/255.0);
            // canvasData[i + 3] = (float)((value%255)/255.0);

            // middle sand
            if (col == 180 && row >= 500) {
                canvasData[i] = (float)((244)/255.0);
                canvasData[i + 1] = (float)((228)/255.0);
                canvasData[i + 2] = (float)((101)/255.0);
                canvasData[i + 3] = (float)(1);
            } else {
                canvasData[i] = (float)((0)/255.0);
                canvasData[i + 1] = (float)((0)/255.0);
                canvasData[i + 2] = (float)((0)/255.0);
                canvasData[i + 3] = (float)((1)/255.0);
            }

            if (row <= 20) {
                canvasData[i] = (float)((117)/255.0);
                canvasData[i + 1] = (float)((116)/255.0);
                canvasData[i + 2] = (float)((103)/255.0);
                canvasData[i + 3] = (float)((1)/255.0);
            }

            i += 4;
            value++;
        }
    }
    std::cout << "Finished generating canvas..."  << std::endl;

    return canvasData;
}

float * updateCanvas(float *currentCanvas, int update) {
    float *canvasData;
    canvasData = new float[(512 * 512) * 4];
    int i = 0;
    for(int row = 0; row < 512; row++) {
        for(int col = 0; col < 512; col++) {

            float currentRed = (float)currentCanvas[i];
            float currentGreen = (float)currentCanvas[i + 1];
            float currentBlue = (float)currentCanvas[i + 2];
            float currentAlpha = (float)currentCanvas[i + 3];

            // test for sand
            if (currentRed == (float)((244)/255.0) && 
                currentGreen == (float)((228)/255.0) && 
                currentBlue == (float)((101)/255.0) &&
                currentAlpha == (float)((1)/255.0)) 
            {
                // check what's below
                float downRed = (float)currentCanvas[(i * 4 * 512)];
                float downBlue = (float)currentCanvas[(i * 4 * 512) + 1];
                float downGreen = (float)currentCanvas[(i * 4 * 512) + 2];
                float downAlpha = (float)currentCanvas[(i * 4 * 512) + 3];

                // draw sand if below is empty
                if (downRed == (float)((0)/255.0) &&
                    downRed == (float)((0)/255.0) &&
                    downRed == (float)((0)/255.0) &&
                    downRed == (float)((0)/255.0))
                {
                    canvasData[i] = (float)((244)/255.0);
                    canvasData[i + 1] = (float)((228)/255.0);
                    canvasData[i + 2] = (float)((101)/255.0);
                    canvasData[i + 3] = (float)(1);
                }
            }

            // test for wall
            if (currentRed == (float)((117)/255.0) &&
                currentGreen == (float)((116)/255.0) &&
                currentBlue == (float)((103)/255.0) &&
                currentAlpha == (float)((1)/255.0)) 
            {
                // wall stays as wall (indestructible)
                canvasData[i] = (float)((117)/255.0);
                canvasData[i + 1] = (float)((116)/255.0);
                canvasData[i + 2] = (float)((103)/255.0);
                canvasData[i + 3] = (float)((1)/255.0);
            }

            // middle sand generator
            if (col == 180 && row >= 500) {
                canvasData[i] = (float)((244)/255.0);
                canvasData[i + 1] = (float)((228)/255.0);
                canvasData[i + 2] = (float)((101)/255.0);
                canvasData[i + 3] = (float)(1);
            } else {
                canvasData[i] = (float)((0)/255.0);
                canvasData[i + 1] = (float)((0)/255.0);
                canvasData[i + 2] = (float)((0)/255.0);
                canvasData[i + 3] = (float)((1)/255.0);
            }

            i += 4;
        }
    }

    return canvasData;
}

void processInput(GLFWwindow *window)
{
    // close window by pressing escape
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

void initializeCanvas()
{
    // fill entire window
    float vertices[] = {
        -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
    };


}