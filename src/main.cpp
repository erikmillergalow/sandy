#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <../include/stb_image.h>

#include <../include/shader.h>

#include <iostream>
#include <time.h>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
float *generateCanvas();
float *updateCanvas(float *currentCanvas, int update);

void processSand(int i, float *currentCanvas, float* canvasData, int step);
void processWater(int i, float *currentCanvas, float* canvasData, int step);

void draw_detection_callback(GLFWwindow *window, int button, int action, int mods);
void draw(float *currentCanvas, double xpos, double ypos, int particleType);

int getParticleType(float *canvas, int index);
int getParticleTypeFromColor(float r, float g, float b, float a);

int downRight(int index);
int downLeft(int index);
int down(int index);
int left(int index);
int right(int index);

void drawParticle(float *canvasLocation, int particleType);
void processInput(GLFWwindow *window);

void initializeCanvas();

// settings
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

int drawPrimary = false;
int drawSecondary = false;
// const unsigned int SCR_WIDTH = 100;
// const unsigned int SCR_HEIGHT = 100;

enum particleTypes{
    UPDATED,
    RANDOM_BLOCK,
    EMPTY,
    WALL,
    SAND,
    WATER
};

void shuffle(size_t *array, size_t n);

int down(int index) {
    return index - (4 * SCR_WIDTH);
}

int downRight(int index) {
    return index - (4 * SCR_WIDTH) + 4;
}

int downLeft(int index) {
    return index - (4 * SCR_WIDTH) - 4;
}

int left(int index) {
    return index - 4;
}

int right (int index) {
    return index + 4;
}

int main()
{
	// glfw: initialize and configure
    std::cout << "Starting..."  << std::endl;
    srand(time(NULL));
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

    glfwSwapInterval(1);

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
    glBindTexture(GL_TEXTURE_2D, texture1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, canvasData);
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

    glfwSetMouseButtonCallback(window, draw_detection_callback);

	// render loop
	while (!glfwWindowShouldClose(window))
	{
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// input
		processInput(window);
        glfwGetCursorPos(window, &xpos, &ypos);
        // std::cout << "xpos: " << xpos << std::endl;
        // std::cout << "ypos: " << ypos << std::endl;
        if (drawPrimary) {
            draw(canvasData, xpos, ypos, SAND);
        } else if (drawSecondary) {
            draw(canvasData, xpos, ypos, WATER);
        }

        canvasUpdate = updateCanvas(canvasData, step);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, canvasUpdate);

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

		glfwPollEvents();

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(window);
	}

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
	// glfw: terminate, clearing all previously allocated GLFWresources.
	//---------------------------------------------------------------
	glfwTerminate();
	return 0;
}

void draw_detection_callback(GLFWwindow * window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            drawPrimary = true;
        } else if (action == GLFW_RELEASE) {
            drawPrimary = false;
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            drawSecondary = true;
        } else if (action == GLFW_RELEASE) {
            drawSecondary = false;
        }
    }
}

float *generateCanvas() {
    std::cout << "Generating canvas..."  << std::endl;
    float *canvasData;
    canvasData = new float[(SCR_WIDTH * SCR_HEIGHT) * 4];
    int value = 0;
    int i = 0;
    for(int row = 0; row < SCR_HEIGHT; row++) {
        for(int col = 0; col < SCR_WIDTH; col++) {
        
            // create wall on the bottom
            if (row < 20) {
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

void draw(float *currentCanvas, double xpos, double ypos, int particleType) {
    int i = 0;

    // access current pixel
    double translatedYPos = std::abs(SCR_HEIGHT - ypos);
    int index = 4 * (int(translatedYPos) *  SCR_WIDTH + int(xpos));

    for (int i = 0; i < 10; i++) {
        drawParticle(&currentCanvas[index + (i * 4)], particleType);
        for (int j = 0; j < 10; j++) {
            drawParticle(&currentCanvas[index + (i * 4) - (SCR_WIDTH * 4 * j)], particleType);
        }
    }

    // return currentCanvas;
}

// used to update cells in random order
void shuffle(size_t *array, size_t n) {
    if (n > 1) {
        for (size_t i = 0; i < n - 1; i++) {
            size_t j = rand() % (n - i) + i;
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

float *updateCanvas(float *currentCanvas, int step) {
    float *canvasData;
    canvasData = new float[(SCR_WIDTH * SCR_HEIGHT) * 4];

    size_t *shuffledIndices;
    shuffledIndices = new size_t[SCR_WIDTH * SCR_HEIGHT]; 
    for (size_t i = 0; i < SCR_WIDTH * SCR_HEIGHT; i++) {
       shuffledIndices[i] = i;
    }

    shuffle(shuffledIndices, SCR_WIDTH * SCR_HEIGHT);

    for (size_t j = 0; j < SCR_WIDTH * SCR_HEIGHT; j++) {
        size_t i = shuffledIndices[j] * 4; 

        int oldParticleType = getParticleType(currentCanvas, i);
        int updatedParticleType = getParticleType(canvasData, i);

        // need to keep track of updated positions here and skip if updated?
        // 
        // need to keep track of oldParticleType even when updated
        if (oldParticleType != UPDATED) { //} || updatedParticleType == EMPTY) {
            if (oldParticleType == WALL) {
                drawParticle(&canvasData[i], WALL);
                drawParticle(&currentCanvas[i], UPDATED);
            } else if (oldParticleType == SAND) {
                processSand(i, currentCanvas, canvasData, step);
            } else if (oldParticleType == WATER) {
                processWater(i, currentCanvas, canvasData, step);
            } else if (oldParticleType == EMPTY) {
                drawParticle(&canvasData[i], EMPTY);
                drawParticle(&currentCanvas[i], UPDATED);
            } else {
                std::cout << "Reached end!!!!!!!!!!!!!!" << std::endl;
            }
        }

    }

    delete(shuffledIndices);
    delete(currentCanvas);
    return canvasData;
}

void processSand(int i, float *currentCanvas, float* canvasData, int step) {
    int downType = getParticleType(currentCanvas, down(i));
    int downLeftType = getParticleType(currentCanvas, downLeft(i));
    int downRightType = getParticleType(currentCanvas, downRight(i));
        
    // drawParticle(&currentCanvas[i], UPDATED);

    // move sand down one pixel if empty space underneath
    if (downType == EMPTY) {
        // fall down
        drawParticle(&canvasData[i], EMPTY);
        drawParticle(&canvasData[down(i)], SAND);
        drawParticle(&currentCanvas[down(i)], UPDATED);

    // check for sand below
    } else if (downType == SAND) {
        
        if (downRightType == EMPTY && downLeftType == EMPTY) {
            if ((rand() % 100) < 50) {
                downRightType = RANDOM_BLOCK;
            } else {
                downLeftType = RANDOM_BLOCK;
            }
        }

        if (downRightType == EMPTY) {
            // fall right
            drawParticle(&canvasData[i], EMPTY);
            drawParticle(&canvasData[downRight(i)], SAND);
            drawParticle(&currentCanvas[downRight(i)], UPDATED);

        } else if (downLeftType == EMPTY) {
            // fall left
            drawParticle(&canvasData[i], EMPTY);
            drawParticle(&canvasData[downLeft(i)], SAND);
            drawParticle(&currentCanvas[downLeft(i)], UPDATED);
        } else if (downLeftType == WATER) {
            // fall left
            drawParticle(&canvasData[i], WATER);
            drawParticle(&canvasData[downLeft(i)], SAND);
            drawParticle(&currentCanvas[downLeft(i)], UPDATED);

        } else if (downRightType == WATER) {
            // fall right
            drawParticle(&canvasData[i], WATER);
            drawParticle(&canvasData[downRight(i)], SAND);
            drawParticle(&currentCanvas[downRight(i)], UPDATED);
        } else {
            // draw sand in same spot (piling up)
            drawParticle(&canvasData[i], SAND);
            drawParticle(&currentCanvas[i], UPDATED);
        }
    } else if (downType == WATER) {
        // sink
        drawParticle(&canvasData[i], WATER);
        drawParticle(&canvasData[down(i)], SAND);
        drawParticle(&currentCanvas[down(i)], UPDATED);
    } else if (downType == WALL) {
        // draw sand
        drawParticle(&canvasData[i], SAND);
    } else if (downType == UPDATED) {
        if (getParticleType(canvasData, down(i)) == EMPTY) {
            // fall down
            drawParticle(&canvasData[i], EMPTY);
            drawParticle(&canvasData[down(i)], SAND);
            drawParticle(&currentCanvas[down(i)], UPDATED);
        } else {
            // draw sand
            drawParticle(&canvasData[i], SAND);
        }
    } else {
        std::cout << "end of sand" << std::endl;
    }
}

enum directions {
    DOWN,
    RIGHT,
    LEFT,
    DOWNRIGHT,
    DOWNLEFT,
};

void processWater(int i, float *currentCanvas, float* canvasData, int step) {
    int downType = getParticleType(currentCanvas, down(i));
    int leftType = getParticleType(currentCanvas, left(i));
    int rightType = getParticleType(currentCanvas, right(i));
    int downLeftType = getParticleType(currentCanvas, downLeft(i));
    int downRightType = getParticleType(currentCanvas, downRight(i));

    if (leftType == UPDATED && getParticleType(canvasData, left(i)) == EMPTY) {
        leftType = EMPTY;
    }
    if (rightType == UPDATED && getParticleType(canvasData, right(i)) == EMPTY) {
        rightType = EMPTY;
    }
    if (downLeftType == UPDATED && getParticleType(canvasData, downLeft(i)) == EMPTY) {
        downLeftType = EMPTY;
    }
    if (downRightType == UPDATED && getParticleType(canvasData, downRight(i)) == EMPTY) {
        downRightType = EMPTY;
    }

    drawParticle(&currentCanvas[i], UPDATED);
    
    // move water down one pixel if empty space underneath
    if (downType == EMPTY) { // && (leftType != EMPTY && rightType != EMPTY && 
        // downLeftType != EMPTY && downRightType != EMPTY)) {
        // fall down
        drawParticle(&canvasData[i], EMPTY);
        drawParticle(&canvasData[down(i)], WATER);
        drawParticle(&currentCanvas[down(i)], UPDATED);

    } else { 

        int chance = rand() % 100;
        // if both left and right are empty, randomly set one to WALL for randomness
        if (downRightType == EMPTY && downLeftType == EMPTY) {
            if (chance < 50) {
                downRightType = RANDOM_BLOCK;
            // } else if (chance < 66) {
            //     downLeftType = RANDOM_BLOCK;
            //     downRightType = RANDOM_BLOCK;
            } else {
                downLeftType = RANDOM_BLOCK;
            }
        }

        if (rightType == EMPTY && leftType == EMPTY) {
            if (chance < 50) {
                rightType = RANDOM_BLOCK;
            // } else if (chance < 66) {
            //     rightType = RANDOM_BLOCK;
            //     leftType = RANDOM_BLOCK;
            } else {
                leftType = RANDOM_BLOCK;
            }
        }

        if (downRightType == EMPTY && downType != WATER) {
            // fall right
            drawParticle(&canvasData[i], EMPTY);
            drawParticle(&canvasData[downRight(i)], WATER);
            drawParticle(&currentCanvas[downRight(i)], UPDATED);
        } else if (downLeftType == EMPTY && downType != WATER) {
            // fall right
            drawParticle(&canvasData[i], EMPTY);
            drawParticle(&canvasData[downLeft(i)], WATER);
            drawParticle(&currentCanvas[downLeft(i)], UPDATED);
        } else if (rightType == EMPTY) {
        // if (rightType == EMPTY) {
            drawParticle(&canvasData[i], EMPTY);
            drawParticle(&canvasData[right(i)], WATER);
            drawParticle(&currentCanvas[right(i)], UPDATED);
        } else if (leftType == EMPTY) {
            drawParticle(&canvasData[i], EMPTY);
            drawParticle(&canvasData[left(i)], WATER);
            drawParticle(&currentCanvas[left(i)], UPDATED);
        } else if (downType == UPDATED && getParticleType(canvasData, down(i)) == EMPTY) {
            drawParticle(&canvasData[i], EMPTY);
            drawParticle(&canvasData[down(i)], WATER);
            drawParticle(&currentCanvas[down(i)], UPDATED);
        } else {
            // draw water in same spot (piling up)
            drawParticle(&canvasData[i], WATER);
            drawParticle(&currentCanvas[i], UPDATED);
        }
    } /*else if (downType == WATER) {
        drawParticle(&canvasData[i], WATER);
    }*/
}

int getParticleType(float *canvas, int index) {
    float red = *(canvas + (index));
    float green = *(canvas + (index) + 1);
    float blue = *(canvas + (index) + 2);
    float alpha = *(canvas + (index) + 3);
    int particleType = getParticleTypeFromColor(red, green, blue, alpha);

    return particleType;
}

int getParticleTypeFromColor(float r, float g, float b, float a) {
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
    } else if (r == (float)((1)/255.0) &&
               g == (float)((1)/255.0) &&
               b == (float)((1)/255.0) &&
               a == (float)(1))
    {
        return UPDATED;
    } else if (r == (float)((2)/255.0) &&
               g == (float)((2)/255.0) &&
               b == (float)((2)/255.0) &&
               a == (float)(2))
    {
        return RANDOM_BLOCK;
    } else {
        // std::cout << "Invalid type" << std::endl;
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
    } else if (particleType == UPDATED) {
        *canvasLocation = (float)((1)/(255.0));
        *(canvasLocation + 1) = (float)((1)/(255.0));
        *(canvasLocation + 2) = (float)((1)/(255.0));
        *(canvasLocation + 3) = (float)(1);
    } else if (particleType == RANDOM_BLOCK) {
        *canvasLocation = (float)((2)/(255.0));
        *(canvasLocation + 1) = (float)((2)/(255.0));
        *(canvasLocation + 2) = (float)((2)/(255.0));
        *(canvasLocation + 3) = (float)(2);
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
