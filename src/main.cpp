#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <../include/stb_image.h>

#include <../include/shader.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
float *generateCanvas();
float *updateCanvas(float *currentCanvas, int update);

void processSand(int i, float *currentCanvas, float* canvasData);

float *draw(float *currentCanvas, double xpos, double ypos);

int getParticleType(float r, float g, float b, float a);
void drawParticle(float *canvasLocation, int particleType);
void processInput(GLFWwindow *window);

void initializeCanvas();

// settings
const unsigned int SCR_WIDTH = 512;
const unsigned int SCR_HEIGHT = 512;

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

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_HEIGHT, SCR_WIDTH, 0, GL_RGBA, GL_FLOAT, canvasData);
    glGenerateMipmap(GL_TEXTURE_2D);

    std::cout << "Texture initialized..."  << std::endl;

    canvasShader.use();
    glUniform1i(glGetUniformLocation(canvasShader.ID, "texture1"), 0);


    // uncomment to activate wireframe mode
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    float *canvasUpdate, *drawUpdate;
    drawUpdate = new float[(SCR_WIDTH * SCR_HEIGHT) * 4];
    canvasUpdate = new float[(SCR_WIDTH * SCR_HEIGHT) * 4];
    int step = 0;
    double xpos, ypos;

	// render loop
	while (!glfwWindowShouldClose(window))
	{
		// input
		processInput(window);
        glfwGetCursorPos(window, &xpos, &ypos);
        int mouseButtonState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        if (mouseButtonState == GLFW_PRESS) {
            drawUpdate = draw(canvasData, xpos, ypos);
            canvasUpdate = updateCanvas(drawUpdate, step);
        } else {
            canvasUpdate = updateCanvas(canvasData, step);
        }

        // update texture
        // canvasUpdate = updateCanvas(drawUpdate, step);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_HEIGHT, SCR_WIDTH, 0, GL_RGBA, GL_FLOAT, canvasUpdate);
        // std::cout << "data:" << canvasData  << std::endl;
        // std::cout << "update:" << canvasUpdate  << std::endl;

        canvasData = canvasUpdate;
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

float *generateCanvas() {
    std::cout << "Generating canvas..."  << std::endl;
    float *canvasData;
    canvasData = new float[(SCR_WIDTH * SCR_HEIGHT) * 4];
    int value = 0;
    int i = 0;
    for(int row = 0; row < SCR_HEIGHT; row++) {
        for(int col = 0; col < SCR_WIDTH; col++) {
            // pattern
            // canvasData[i] = (float)((value%255)/255.0);
            // canvasData[i + 1] = (float)((value%255)/255.0);
            // canvasData[i + 2] = (float)((value%255)/255.0);
            // canvasData[i + 3] = (float)((value%255)/255.0);

            // middle sand
            // if (col == 180 && row >= SCR_HEIGHT - 2) {
            //     canvasData[i] = (float)((244)/255.0);
            //     canvasData[i + 1] = (float)((228)/255.0);
            //     canvasData[i + 2] = (float)((101)/255.0);
            //     canvasData[i + 3] = (float)(1);
            // } else {
            //     canvasData[i] = (float)((0));
            //     canvasData[i + 1] = (float)(0);
            //     canvasData[i + 2] = (float)(0);
            //     canvasData[i + 3] = (float)(1);
            // }

            // create wall on the bottom
            if (row <= 20) {
                canvasData[i] = (float)((117)/255.0);
                canvasData[i + 1] = (float)((116)/255.0);
                canvasData[i + 2] = (float)((103)/255.0);
                canvasData[i + 3] = (float)(1);
            }

            i += 4;
            value++;
        }
    }
    std::cout << "Finished generating canvas..."  << std::endl;

    return canvasData;
}

enum particleTypes{
    EMPTY,
    WALL,
    SAND,
    WATER
};

// struct SAND {
//     float r = (float)((244)/(255.0));
//     float g = (float)((228)/(255.0));
//     float b = (float)((101)/(255.0));
//     float a = (float)(1);
// }

float *draw(float *currentCanvas, double xpos, double ypos) {
    float *canvasData;
    canvasData = new float[(SCR_WIDTH * SCR_HEIGHT) * 4];
    int i = 0;
    std::cout << "X: " << xpos << std::endl;
    std::cout << "Y: " << ypos << std::endl;
    
    for (int row = 0; row < SCR_HEIGHT; row++) {
        for (int col = 0; col < SCR_WIDTH; col++) {
            // access current pixel
            float currentRed = *(currentCanvas + (i));
            float currentGreen = *(currentCanvas + (i) + 1);
            float currentBlue = *(currentCanvas + (i) + 2);
            float currentAlpha = *(currentCanvas + (i) + 3);
            int currentType = getParticleType(currentRed, currentGreen, currentBlue, currentAlpha);

            double translatedYPos = std::abs(ypos - SCR_HEIGHT);
            if ((std::abs(row - translatedYPos) <  5) && (std::abs(col - xpos) < 5)) {
                drawParticle(&canvasData[i], SAND);
            } else {
                // copy last frame
                drawParticle(&canvasData[i], currentType);
            }

            i += 4;
        }
    }

    delete(currentCanvas);
    return canvasData;
}

float *updateCanvas(float *currentCanvas, int update) {
    float *canvasData;
    canvasData = new float[(SCR_WIDTH * SCR_HEIGHT) * 4];
    int i = 0;
    for (int row = 0; row < SCR_HEIGHT; row++) {
        for (int col = 0; col < SCR_WIDTH; col++) {

            // access current pixel
            float currentRed = *(currentCanvas + (i));
            float currentGreen = *(currentCanvas + (i) + 1);
            float currentBlue = *(currentCanvas + (i) + 2);
            float currentAlpha = *(currentCanvas + (i) + 3);
            int currentType = getParticleType(currentRed, currentGreen, currentBlue, currentAlpha);

            // test for wall
            if (currentType == WALL)
            {
                // wall stays as wall (indestructible)
                drawParticle(&canvasData[i], WALL);
            } else {
                // test for sand
                if (currentType == SAND) {
                    processSand(i, currentCanvas, canvasData);

                    // // check what's below
                    // float downRed = *(canvasData + (i) - (4 * SCR_HEIGHT));
                    // float downGreen = *(canvasData + (i) - (4 * SCR_HEIGHT) + 1);
                    // float downBlue = *(canvasData + (i) - (4 * SCR_HEIGHT) + 2);
                    // float downAlpha = *(canvasData + (i) - (4 * SCR_HEIGHT) + 3);
                    // int downType = getParticleType(downRed, downGreen, downBlue, downAlpha);

                    // // move sand down one pixel if empty space underneath
                    // if (downType == EMPTY) {
                    //     // fall down
                    //     drawParticle(&canvasData[i], EMPTY);
                    //     drawParticle(&canvasData[(i - (4 * SCR_HEIGHT))], SAND);

                    // // check for sand below
                    // } else if (downType == SAND) {
                    //     // check for space to the left
                    //     float downLeftRed = *(canvasData + (i - (4 * SCR_HEIGHT) - 4));
                    //     float downLeftGreen = *(canvasData + (i - (4 * SCR_HEIGHT) - 3));
                    //     float downLeftBlue = *(canvasData + (i - (4 * SCR_HEIGHT) - 2));
                    //     float downLeftAlpha = *(canvasData + (i - (4 * SCR_HEIGHT) - 1));
                    //     int downLeftType = getParticleType(downLeftRed, downLeftGreen, downLeftBlue, downLeftAlpha);
                        
                    //     // check for space to the right
                    //     float downRightRed = *(currentCanvas + (i - (4 * SCR_HEIGHT) + 4));
                    //     float downRightGreen = *(currentCanvas + (i - (4 * SCR_HEIGHT) + 5));
                    //     float downRightBlue = *(currentCanvas + (i - (4 * SCR_HEIGHT) + 6));
                    //     float downRightAlpha = *(currentCanvas + (i - (4 * SCR_HEIGHT) + 7));
                    //     int downRightType = getParticleType(downRightRed, downRightGreen, downRightBlue, downRightAlpha);

                    //     if (downLeftType == EMPTY) {
                    //         // fall left
                    //         drawParticle(&canvasData[i], EMPTY);
                    //         drawParticle(&canvasData[(i - (4 * SCR_HEIGHT)) - 4], SAND);

                    //     } else if (downRightType == EMPTY) {
                    //         // fall right
                    //         drawParticle(&canvasData[i], EMPTY);
                    //         drawParticle(&canvasData[(i - (4 * SCR_HEIGHT)) + 4], SAND);
                    //     } else {
                    //         // draw sand in same spot (piling up)
                    //         drawParticle(&canvasData[i], SAND);
                    //     }
                    // }

                    // // draw sand if wall is below
                    // if (downType == WALL) {
                    //     // draw sand
                    //     drawParticle(&canvasData[i], SAND);
                    // }
                } else if (currentType == WATER) {
                    
                
                } else if (currentType == EMPTY) {
                    // draw empty
                    drawParticle(&canvasData[i], EMPTY);
                }

                // middle sand generator
                // if (col == 180 && row >= SCR_HEIGHT - 1) {
                //     drawParticle(&canvasData[i], SAND);
                // }
            }

            i += 4;
        }
    }

    delete(currentCanvas);
    return canvasData;
}

void processSand(int i, float *currentCanvas, float* canvasData) {
    // check what's below
    float downRed = *(canvasData + (i) - (4 * SCR_HEIGHT));
    float downGreen = *(canvasData + (i) - (4 * SCR_HEIGHT) + 1);
    float downBlue = *(canvasData + (i) - (4 * SCR_HEIGHT) + 2);
    float downAlpha = *(canvasData + (i) - (4 * SCR_HEIGHT) + 3);
    int downType = getParticleType(downRed, downGreen, downBlue, downAlpha);

    // move sand down one pixel if empty space underneath
    if (downType == EMPTY) {
        // fall down
        drawParticle(&canvasData[i], EMPTY);
        drawParticle(&canvasData[(i - (4 * SCR_HEIGHT))], SAND);

    // check for sand below
    } else if (downType == SAND) {
        // check for space to the left
        float downLeftRed = *(canvasData + (i - (4 * SCR_HEIGHT) - 4));
        float downLeftGreen = *(canvasData + (i - (4 * SCR_HEIGHT) - 3));
        float downLeftBlue = *(canvasData + (i - (4 * SCR_HEIGHT) - 2));
        float downLeftAlpha = *(canvasData + (i - (4 * SCR_HEIGHT) - 1));
        int downLeftType = getParticleType(downLeftRed, downLeftGreen, downLeftBlue, downLeftAlpha);
        
        // check for space to the right
        float downRightRed = *(currentCanvas + (i - (4 * SCR_HEIGHT) + 4));
        float downRightGreen = *(currentCanvas + (i - (4 * SCR_HEIGHT) + 5));
        float downRightBlue = *(currentCanvas + (i - (4 * SCR_HEIGHT) + 6));
        float downRightAlpha = *(currentCanvas + (i - (4 * SCR_HEIGHT) + 7));
        int downRightType = getParticleType(downRightRed, downRightGreen, downRightBlue, downRightAlpha);

        if (downLeftType == EMPTY) {
            // fall left
            drawParticle(&canvasData[i], EMPTY);
            drawParticle(&canvasData[(i - (4 * SCR_HEIGHT)) - 4], SAND);

        } else if (downRightType == EMPTY) {
            // fall right
            drawParticle(&canvasData[i], EMPTY);
            drawParticle(&canvasData[(i - (4 * SCR_HEIGHT)) + 4], SAND);
        } else {
            // draw sand in same spot (piling up)
            drawParticle(&canvasData[i], SAND);
        }
    }

    // draw sand if wall is below
    if (downType == WALL) {
        // draw sand
        drawParticle(&canvasData[i], SAND);
    }
}

int getParticleType(float r, float g, float b, float a) {
    if (r == (float)((0)/255.0) &&
        g == (float)((0)/255.0) &&
        b == (float)((0)/255.0) &&
        a == (float)(1))
    {
        return EMPTY;
    } else if (r == (float)((117)/255.0) &&
               g == (float)((116)/255.0) &&
               b == (float)((103)/255.0) &&
               a == (float)(1))
    {
        return WALL;
    } else if (r == (float)((244)/255.0) &&
               g == (float)((228)/255.0) &&
               b == (float)((101)/255.0) &&
               a == (float)(1))
    {
        return SAND;
    } else {
        return EMPTY;
    }
}

void drawParticle(float *canvasLocation, int particleType)
{
    if (particleType == EMPTY) {
        *canvasLocation = (float)((0)/255.0);
        *(canvasLocation + 1) = (float)((0)/255.0);
        *(canvasLocation + 2) = (float)((0)/255.0);
        *(canvasLocation + 3) = (float)(1);
    } else if (particleType == WALL) {
        *canvasLocation = ((117)/255.0);
        *(canvasLocation + 1) = (float)((116)/255.0);
        *(canvasLocation + 2) = (float)((103)/255.0);
        *(canvasLocation + 3) = (float)(1);
    } else if (particleType == SAND) {
        *canvasLocation = (float)((244)/(255.0));
        *(canvasLocation + 1) = (float)((228)/(255.0));
        *(canvasLocation + 2) = (float)((101)/(255.0));
        *(canvasLocation + 3) = (float)(1);
    } else if (particleType == WATER) {

    }

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