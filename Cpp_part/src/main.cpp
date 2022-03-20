#include <memory>
#include <fstream>
#include <iostream>
#include <string>
#include <string.h>

#include <GL/freeglut.h>

#include "texture.h"
#include "gl.h"

#include <entrypoint.h>


void drawTexture(const GLTexture& tex, int texWidth, int texHeight)
{
	int width = glutGet(GLUT_WINDOW_WIDTH);
	int height = glutGet(GLUT_WINDOW_HEIGHT);

	float percentWidth = (float)texWidth * 2 / (float)width;
	float percentHeight = (float)texHeight * 2 / (float)height;

	tex.bindTo(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	// Front Face
    glTexCoord2f(0.0f, 0.0f); glVertex3f( percentWidth, 1.f - percentHeight * 2.f	,  0.f);  // Bottom Left Of The Texture and Quad
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-percentWidth, 1.f - percentHeight * 2.f	,  0.f);  // Bottom Right Of The Texture and Quad
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-percentWidth, 1.f							,  0.f);  // Top Right Of The Texture and Quad
    glTexCoord2f(0.0f, 1.0f); glVertex3f( percentWidth, 1.f							,  0.f);  // Top Left Of The Texture and Quad

	glEnd();
}

WomboTexture epicTex;

void my_init()
{
	epicTex = WomboTexture("epic.bmp", false);

	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
}

void display()
{	
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(0.f, 0.f, 0.f);
	glColor3f(1.f, 0.f, 1.f);
	glutSolidCube(0.4);
	glColor3f(1.f, 1.f, 1.f);
	drawTexture(epicTex.getGLTex(), epicTex.getCpuTex().getWidth(), epicTex.getCpuTex().getHeight());

	glColor4f(1.f, 0.f, 0.f, 1.f);
	glRasterPos2i(100, 120);
	glutBitmapString(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)"big chungus");

	glutSwapBuffers();
}

void setTexPixel(int value)
{
	(void)value;

	std::cout << "Setting pixels" << std::endl;
	checkForGlError("Before setting pixel");
	epicTex.setArea(15, 15, 25, 25, Colors::Orange8);
	checkForGlError("After setting pixel");
	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	epic_exported_function();

	glutInit(&argc, argv);

	glutInitWindowSize(800, 600);

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);

	glutCreateWindow(argv[0]);

	my_init();

	glutDisplayFunc(display);

	glutTimerFunc(3000, setTexPixel, 0);

	glutMainLoop();

	return 0;
}

