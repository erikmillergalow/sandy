#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <../include/stb_image.h>

#include <../include/shader.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
float *generateCanvas();
float *updateCanvas(float *currentCanvas, int update);

void processSand(int i, float *currentCanvas, float* canvasData, int step);
void processWater(int i, float *currentCanvas, float* canvasData, int step);

float *draw(float *currentCanvas, double xpos, double ypos, int particleType);

int getParticleType(float r, float g, float b, float a);
void drawParticle(float *canvasLocation, int particleType);
void processInput(GLFWwindow *window);

void initializeCanvas();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// const unsigned int SCR_WIDTH = 100;
// const unsigned int SCR_HEIGHT = 100;

enum particleTypes{
    EMPTY,
    WALL,
    SAND,
    WATER
};

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
        int leftMouseButtonState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        int rightMouseButtonState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
        if (leftMouseButtonState == GLFW_PRESS) {
            drawUpdate = draw(canvasData, xpos, ypos, SAND);
            canvasUpdate = updateCanvas(drawUpdate, step);
        } else if (rightMouseButtonState == GLFW_PRESS) {
            drawUpdate = draw(canvasData, xpos, ypos, WATER);
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
        step++;

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
    for(int row = 0; row < SCR_WIDTH; row++) {
        for(int col = 0; col < SCR_HEIGHT; col++) {

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

// struct SAND {
//     float r = (float)((244)/(255.0));
//     float g = (float)((228)/(255.0));
//     float b = (float)((101)/(255.0));
//     float a = (float)(1);
// }

float *draw(float *currentCanvas, double xpos, double ypos, int particleType) {
    float *canvasData;
    canvasData = new float[(SCR_WIDTH * SCR_HEIGHT) * 4];
    int i = 0;
    std::cout << "X: " << xpos << std::endl;
    std::cout << "Y: " << ypos << std::endl;
    
    for (int row = 0; row < SCR_WIDTH; row++) {
        for (int col = 0; col < SCR_HEIGHT; col++) {
    //         // access current pixel
            double translatedYPos = std::abs(SCR_WIDTH - ypos);
            int index = (int)((xpos * 4) + (translatedYPos *  4 * SCR_HEIGHT));

            float currentRed = *(currentCanvas + (i));
            float currentGreen = *(currentCanvas + (i) + 1);
            float currentBlue = *(currentCanvas + (i) + 2);
            float currentAlpha = *(currentCanvas + (i) + 3);
            int currentType = getParticleType(currentRed, currentGreen, currentBlue, currentAlpha);

            // if (currentType == EMPTY) {
            //     drawParticle(&currentCanvas[index], particleType);
            //     drawParticle(&currentCanvas[index + 4], particleType);
            //     drawParticle(&currentCanvas[index + 8], particleType);
            //     drawParticle(&currentCanvas[index + 12], particleType);
            //     drawParticle(&currentCanvas[index + 16], particleType);
            //     drawParticle(&currentCanvas[index + (SCR_HEIGHT * 4) ], particleType);
            //     drawParticle(&currentCanvas[index + ((SCR_HEIGHT * 4) + 4)], particleType);
            //     drawParticle(&currentCanvas[index + ((SCR_HEIGHT * 4) + 8)], particleType);
            //     drawParticle(&currentCanvas[index + ((SCR_HEIGHT * 4) + 12)], particleType);
            //     drawParticle(&currentCanvas[index + ((SCR_HEIGHT * 4) + 16)], particleType);
            // } else {
            //     // copy last frame
            //     drawParticle(&currentCanvas[index], currentType); 
            // }
            if ((std::abs(row - translatedYPos) < 5) && (std::abs(col - xpos) < 5)) {
                // std::cout << "\nrow: " << row << std::endl;
                // std::cout << "ypos: " << ypos << std::endl;
                // std::cout << "translatedY: " << translatedYPos << std::endl;
                // std::cout << "\nxpos: " << xpos << std::endl;
                // std::cout << "col: " << col << std::endl;
                drawParticle(&canvasData[i], particleType);
            } else {
                // copy last frame
                drawParticle(&canvasData[i], currentType);
            }

            i += 4;
        }
    }

    delete(currentCanvas);
    // return currentCanvas;
    return canvasData;
}

float *updateCanvas(float *currentCanvas, int step) {
    float *canvasData;
    canvasData = new float[(SCR_WIDTH * SCR_HEIGHT) * 4];
    int i = 0;
    for (int row = 0; row < SCR_WIDTH; row++) {
        for (int col = 0; col < SCR_HEIGHT; col++) {

            // access current pixel
            float currentRed = *(currentCanvas + (i));
            float currentGreen = *(currentCanvas + (i) + 1);
            float currentBlue = *(currentCanvas + (i) + 2);
            float currentAlpha = *(currentCanvas + (i) + 3);
            int oldParticleType = getParticleType(currentRed, currentGreen, currentBlue, currentAlpha);

            float canvasRed = *(canvasData + (i));
            float canvasGreen = *(canvasData + (i) + 1);
            float canvasBlue = *(canvasData + (i) + 2);
            float canvasAlpha = *(canvasData + (i) + 3);
            int updatedParticleType = getParticleType(canvasRed, canvasGreen, canvasBlue, canvasAlpha);

            // need to check if particle from last update moved into this position
            if (oldParticleType == EMPTY && updatedParticleType != EMPTY) {
                drawParticle(&canvasData[i], updatedParticleType);
            } else {
                if (oldParticleType == WALL) {
                    drawParticle(&canvasData[i], WALL);
                } else if (oldParticleType == SAND) {
                    processSand(i, currentCanvas, canvasData, step);
                } else if (oldParticleType == WATER) {
                    processWater(i, currentCanvas, canvasData, step);
                } else if (oldParticleType == EMPTY) {
                    drawParticle(&canvasData[i], EMPTY);
                } else {
                    std::cout << "Reached end!!!!!!!!!!!!!!" << std::endl;
                }
            }

            i += 4;
        }
    }

    delete(currentCanvas);
    return canvasData;
}

void processSand(int i, float *currentCanvas, float* canvasData, int step) {
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
        float downRightRed = *(canvasData + (i - (4 * SCR_HEIGHT) + 4));
        float downRightGreen = *(canvasData + (i - (4 * SCR_HEIGHT) + 5));
        float downRightBlue = *(canvasData + (i - (4 * SCR_HEIGHT) + 6));
        float downRightAlpha = *(canvasData + (i - (4 * SCR_HEIGHT) + 7));
        int downRightType = getParticleType(downRightRed, downRightGreen, downRightBlue, downRightAlpha);

        if (downRightType == EMPTY) {
            // fall right
            drawParticle(&canvasData[i], EMPTY);
            drawParticle(&canvasData[(i - (4 * SCR_HEIGHT)) + 4], SAND);

        } else if (downLeftType == EMPTY) {
            // fall left
            drawParticle(&canvasData[i], EMPTY);
            drawParticle(&canvasData[(i - (4 * SCR_HEIGHT)) - 4], SAND);
        } else if (downLeftType == WATER) {
            // fall left
            drawParticle(&canvasData[i], WATER);
            drawParticle(&canvasData[(i - (4 * SCR_HEIGHT)) - 4], SAND);

        } else if (downRightType == WATER) {
            // fall right
            drawParticle(&canvasData[i], WATER);
            drawParticle(&canvasData[(i - (4 * SCR_HEIGHT)) + 4], SAND);
        } else {
            // draw sand in same spot (piling up)
            drawParticle(&canvasData[i], SAND);
        }
    } else if (downType == WATER) {
        // sink
        drawParticle(&canvasData[i], WATER);
        drawParticle(&canvasData[(i - (4 * SCR_HEIGHT))], SAND);
    } else if (downType == WALL) {
        // draw sand
        drawParticle(&canvasData[i], SAND);
    }
}

void processWater(int i, float *currentCanvas, float* canvasData, int step) {
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
        drawParticle(&canvasData[(i - (4 * SCR_HEIGHT))], WATER);

    } else if (downType == SAND || downType == WALL || downType == WATER) {
        // check for space to the left
        float leftRed = *(canvasData + (i - 4));
        float leftGreen = *(canvasData + (i - 3));
        float leftBlue = *(canvasData + (i - 2));
        float leftAlpha = *(canvasData + (i - 1));
        int leftType = getParticleType(leftRed, leftGreen, leftBlue, leftAlpha);

        // check for space to the right
        float rightRed = *(currentCanvas + (i + 4));
        float rightGreen = *(currentCanvas + (i + 5));
        float rightBlue = *(currentCanvas + (i + 6));
        float rightAlpha = *(currentCanvas + (i + 7));
        int rightType = getParticleType(rightRed, rightGreen, rightBlue, rightAlpha);

        // check for space to the downward left
        float downLeftRed = *(canvasData + (i - (4 * SCR_HEIGHT) - 4));
        float downLeftGreen = *(canvasData + (i - (4 * SCR_HEIGHT) - 3));
        float downLeftBlue = *(canvasData + (i - (4 * SCR_HEIGHT) - 2));
        float downLeftAlpha = *(canvasData + (i - (4 * SCR_HEIGHT) - 1));
        int downLeftType = getParticleType(downLeftRed, downLeftGreen, downLeftBlue, downLeftAlpha);
        
        // check for space to the downward right
        float downRightRed = *(canvasData + (i - (4 * SCR_HEIGHT) + 4));
        float downRightGreen = *(canvasData + (i - (4 * SCR_HEIGHT) + 5));
        float downRightBlue = *(canvasData + (i - (4 * SCR_HEIGHT) + 6));
        float downRightAlpha = *(canvasData + (i - (4 * SCR_HEIGHT) + 7));
        int downRightType = getParticleType(downRightRed, downRightGreen, downRightBlue, downRightAlpha);

        if (downRightType == EMPTY) {
            // fall right
            // std::cout << "fall left" << std::endl;
            drawParticle(&canvasData[i], EMPTY);
            drawParticle(&canvasData[(i - (4 * SCR_HEIGHT)) + 4], WATER);
        } else if (downLeftType == EMPTY) {
            // fall right
            // std::cout << "fall left" << std::endl;
            drawParticle(&canvasData[i], EMPTY);
            drawParticle(&canvasData[(i - (4 * SCR_HEIGHT)) - 4], WATER);
        } else if (rightType == EMPTY) {
            // std::cout << "move right" << std::endl;
            drawParticle(&canvasData[i], EMPTY);
            drawParticle(&canvasData[i + 4], WATER);
        } else if (leftType == EMPTY) {
            // std::cout << "move left" << std::endl;
            drawParticle(&canvasData[i], EMPTY);
            drawParticle(&canvasData[i - 4], WATER);
        } else {
            // draw water in same spot (piling up)
            // std::cout << "stay still" << std::endl;
            drawParticle(&canvasData[i], WATER);
        }
    } /*else if (downType == WATER) {
        drawParticle(&canvasData[i], WATER);
    }*/
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
    } else if (r == (float)((17)/255.0) &&
               g == (float)((65)/255.0) &&
               b == (float)((166)/255.0) &&
               a == (float)(1))
    {
        return WATER;
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
        *canvasLocation = (float)((17)/(255.0));
        *(canvasLocation + 1) = (float)((65)/(255.0));
        *(canvasLocation + 2) = (float)((166)/(255.0));
        *(canvasLocation + 3) = (float)(1);
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