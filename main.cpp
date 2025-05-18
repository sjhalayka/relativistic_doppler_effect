// Relativistic Doppler Effect Visualization
// Comparing Galactic and Keplerian Orbits
// Using OpenGL, GLUT, GLEW, and GLM



#define _CRT_SECURE_NO_WARNINGS


#include <GL/glew.h>
#pragma comment(lib, "glew32")


#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include <random>
#include <iostream>

// Constants
const int NUM_STARS = 100000;
const float GALAXY_RADIUS = 15.0f;
const float MAX_VELOCITY = 0.5f; // as fraction of c (speed of light)
const float SPEED_OF_LIGHT = 1.0f; // normalized
const float FLAT_ROTATION_VELOCITY = 0.5f; // as fraction of c
const float OBSERVER_POSITION_Z = 20.0f;

// Display settings
int windowWidth = 1200;
int windowHeight = 600;
bool showKeplerian = true;
bool showFlatRotation = true;
float viewAngle = 0.0f;
float observerVelocity = 0.0f; // Observer's velocity as fraction of c

// Star data structures
struct Star {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;
    float dopplerShiftedColor[3];
};

std::vector<Star> keplerianStars;
std::vector<Star> flatRotationStars;

// Random number generator
std::random_device rd;
std::mt19937 gen(rd());

const double M_PI = 4.0 * atan(1.0);

std::uniform_real_distribution<float> angleDistribution(0.0f, 2.0f * M_PI);
std::uniform_real_distribution<float> radiusDistribution(0.1f, GALAXY_RADIUS);
std::uniform_real_distribution<float> heightDistribution(-0.5f, 0.5f);

// Function to convert wavelength to RGB color
void wavelengthToRGB(float wavelength, float rgb[3]) {
    // Simplified visible spectrum approximation (400nm to 700nm)
    // Wavelength is normalized to 0.0-1.0 range where 0.0 is 400nm and 1.0 is 700nm

    if (wavelength <= 0.25f) { // Violet to blue (400-475nm)
        rgb[0] = 0.5f * (wavelength / 0.25f);
        rgb[1] = 0.0f;
        rgb[2] = 0.5f + 0.5f * (wavelength / 0.25f);
    }
    else if (wavelength <= 0.4f) { // Blue to cyan (475-500nm)
        rgb[0] = 0.0f;
        rgb[1] = (wavelength - 0.25f) / 0.15f;
        rgb[2] = 1.0f;
    }
    else if (wavelength <= 0.55f) { // Cyan to green (500-570nm)
        rgb[0] = 0.0f;
        rgb[1] = 1.0f;
        rgb[2] = 1.0f - (wavelength - 0.4f) / 0.15f;
    }
    else if (wavelength <= 0.6f) { // Green to yellow (570-590nm)
        rgb[0] = (wavelength - 0.55f) / 0.05f;
        rgb[1] = 1.0f;
        rgb[2] = 0.0f;
    }
    else if (wavelength <= 0.75f) { // Yellow to red (590-650nm)
        rgb[0] = 1.0f;
        rgb[1] = 1.0f - (wavelength - 0.6f) / 0.15f;
        rgb[2] = 0.0f;
    }
    else { // Red (650-700nm)
        rgb[0] = 1.0f;
        rgb[1] = 0.0f;
        rgb[2] = 0.0f;
    }
}

// Calculate relativistic Doppler shift
float calculateRelativisticDopplerShift(const glm::vec3& starVelocity, const glm::vec3& observerVelocity) {
    // Calculate relative velocity along the line of sight
    glm::vec3 lineOfSight = glm::normalize(glm::vec3(0.0f, 0.0f, OBSERVER_POSITION_Z) - starVelocity);
    float relativeVelocity = glm::dot(starVelocity - observerVelocity, lineOfSight);

    // Relativistic Doppler shift formula: λ' = λ * sqrt((1 + v/c) / (1 - v/c))
    // For wavelength, redshift (moving away) gives larger wavelength, blueshift (moving toward) gives smaller wavelength
    float beta = relativeVelocity / SPEED_OF_LIGHT;

    // Prevent division by zero or negative square root
    if (beta >= 1.0f) beta = 0.99f;
    if (beta <= -1.0f) beta = -0.99f;

    float dopplerFactor = sqrt((1.0f + beta) / (1.0f - beta));
    return dopplerFactor;
}

// Initialize star positions and velocities
void initializeStars() {
    keplerianStars.clear();
    flatRotationStars.clear();

    for (int i = 0; i < NUM_STARS; i++) {
        float angle = angleDistribution(gen);
        float radius = radiusDistribution(gen);
        float height = heightDistribution(gen);

        // Position (same for both models initially)
        glm::vec3 position(radius * cos(angle), height, radius * sin(angle));

        // Base star color (wavelength in visible spectrum range, normalized to 0-1)
        float wavelength = 0.5f; // Middle of visible spectrum
        float baseColor[3];
        wavelengthToRGB(wavelength, baseColor);

        // Create Keplerian star
        Star keplerianStar;
        keplerianStar.position = position;

        // Keplerian orbital velocity (proportional to 1/sqrt(r))
        float keplerianSpeed = MAX_VELOCITY * sqrt(GALAXY_RADIUS / (radius + 0.1f));
        float tangentX = -sin(angle);
        float tangentZ = cos(angle);
        keplerianStar.velocity = glm::vec3(keplerianSpeed * tangentX, 0.0f, keplerianSpeed * tangentZ);
        keplerianStar.color = glm::vec3(baseColor[0], baseColor[1], baseColor[2]);

        // Calculate Doppler shift for Keplerian model
        float keplerianDopplerFactor = calculateRelativisticDopplerShift(keplerianStar.velocity,
            glm::vec3(0.0f, 0.0f, observerVelocity));
        float shiftedWavelength = wavelength * keplerianDopplerFactor;
        // Clamp to visible spectrum
        shiftedWavelength = glm::clamp(shiftedWavelength, 0.0f, 1.0f);
        wavelengthToRGB(shiftedWavelength, keplerianStar.dopplerShiftedColor);

        keplerianStars.push_back(keplerianStar);

        // Create flat rotation curve star
        Star flatStar;
        flatStar.position = position;

        // Flat rotation curve (constant velocity regardless of radius)
        flatStar.velocity = glm::vec3(FLAT_ROTATION_VELOCITY * tangentX, 0.0f, FLAT_ROTATION_VELOCITY * tangentZ);
        flatStar.color = glm::vec3(baseColor[0], baseColor[1], baseColor[2]);

        // Calculate Doppler shift for flat rotation model
        float flatDopplerFactor = calculateRelativisticDopplerShift(flatStar.velocity,
            glm::vec3(0.0f, 0.0f, observerVelocity));
        shiftedWavelength = wavelength * flatDopplerFactor;
        // Clamp to visible spectrum
        shiftedWavelength = glm::clamp(shiftedWavelength, 0.0f, 1.0f);
        wavelengthToRGB(shiftedWavelength, flatStar.dopplerShiftedColor);

        flatRotationStars.push_back(flatStar);
    }
}

// Update Doppler shifts based on current view and observer velocity
void updateDopplerShifts() {
    glm::vec3 observerVel(0.0f, 0.0f, observerVelocity);

    for (auto& star : keplerianStars) {
        float dopplerFactor = calculateRelativisticDopplerShift(star.velocity, observerVel);
        float baseWavelength = 0.5f; // Middle of visible spectrum
        float shiftedWavelength = baseWavelength * dopplerFactor;
        shiftedWavelength = glm::clamp(shiftedWavelength, 0.0f, 1.0f);
        wavelengthToRGB(shiftedWavelength, star.dopplerShiftedColor);
    }

    for (auto& star : flatRotationStars) {
        float dopplerFactor = calculateRelativisticDopplerShift(star.velocity, observerVel);
        float baseWavelength = 0.5f; // Middle of visible spectrum
        float shiftedWavelength = baseWavelength * dopplerFactor;
        shiftedWavelength = glm::clamp(shiftedWavelength, 0.0f, 1.0f);
        wavelengthToRGB(shiftedWavelength, star.dopplerShiftedColor);
    }
}

// Display function
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Left viewport - Keplerian model
    if (showKeplerian) {
        glViewport(0, 0, windowWidth / 2, windowHeight);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0f, (float)(windowWidth / 2) / (float)windowHeight, 0.1f, 100.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(0.0f, 10.0f, OBSERVER_POSITION_Z,  // Eye position
            0.0f, 0.0f, 0.0f,   // Look at position
            0.0f, 1.0f, 0.0f);  // Up vector

      //  glRotatef(viewAngle, 0.0f, 1.0f, 0.0f);

        // Draw Keplerian stars
        glPointSize(2.0f);
        glBegin(GL_POINTS);
        for (const auto& star : keplerianStars) {
            glColor3fv(star.dopplerShiftedColor);
            glVertex3fv(glm::value_ptr(star.position));
        }
        glEnd();

        // Draw text label
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos3f(-5.0f, 8.0f, 0.0f);
        const char* keplerianLabel = "Keplerian Orbit Model";
        for (const char* c = keplerianLabel; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }
    }

    // Right viewport - Flat rotation curve model
    if (showFlatRotation) {
        glViewport(windowWidth / 2, 0, windowWidth / 2, windowHeight);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0f, (float)(windowWidth / 2) / (float)windowHeight, 0.1f, 100.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(0.0f, 10.0f, OBSERVER_POSITION_Z,  // Eye position
            0.0f, 0.0f, 0.0f,   // Look at position
            0.0f, 1.0f, 0.0f);  // Up vector

       // glRotatef(viewAngle, 0.0f, 1.0f, 0.0f);

        // Draw flat rotation curve stars
        glPointSize(2.0f);
        glBegin(GL_POINTS);
        for (const auto& star : flatRotationStars) {
            glColor3fv(star.dopplerShiftedColor);
            glVertex3fv(glm::value_ptr(star.position));
        }
        glEnd();

        // Draw text label
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos3f(-5.0f, 8.0f, 0.0f);
        const char* flatLabel = "Flat Rotation Curve Model";
        for (const char* c = flatLabel; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }
    }

    // Draw information about observer velocity and controls
    glViewport(0, 0, windowWidth, windowHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, windowWidth, 0, windowHeight, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(10, windowHeight - 20);

    char velocityInfo[100];
    sprintf(velocityInfo, "Observer Velocity: %.2fc | View Angle: %.1f | Use 'W/S' for velocity, 'A/D' for rotation",
        observerVelocity, viewAngle);

    for (const char* c = velocityInfo; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // Draw Doppler explanation and color scale
    glRasterPos2f(10, 20);
    const char* dopplerInfo = "Redshift = Moving Away (Redder) | Blueshift = Moving Toward (Bluer)";
    for (const char* c = dopplerInfo; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // Draw color scale
    //const int scaleWidth = 400;
    //const int scaleHeight = 20;
    //const int scaleX = (windowWidth - scaleWidth) / 2;
    //const int scaleY = 50;

    //glBegin(GL_QUADS);
    //for (int i = 0; i < scaleWidth; i++) {
    //    float wavelength = (float)i / scaleWidth;
    //    float rgb[3];
    //    wavelengthToRGB(wavelength, rgb);
    //    glColor3fv(rgb);

    //    float x = scaleX + i;
    //    glVertex2f(x, scaleY);
    //    glVertex2f(x + 1, scaleY);
    //    glVertex2f(x + 1, scaleY + scaleHeight);
    //    glVertex2f(x, scaleY + scaleHeight);
    //}
    //glEnd();

    //// Draw labels for color scale
    //glColor3f(1.0f, 1.0f, 1.0f);
    //glRasterPos2f(scaleX - 60, scaleY + scaleHeight / 2);
    //const char* blueLabel = "Blueshift";
    //for (const char* c = blueLabel; *c != '\0'; c++) {
    //    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
    //}

    //glRasterPos2f(scaleX + scaleWidth + 10, scaleY + scaleHeight / 2);
    //const char* redLabel = "Redshift";
    //for (const char* c = redLabel; *c != '\0'; c++) {
    //    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
    //}

    glutSwapBuffers();
}

// Keyboard function
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 'w': case 'W':
        observerVelocity += 0.01f;
        if (observerVelocity > 0.9f) observerVelocity = 0.9f;
        updateDopplerShifts();
        break;
    case 's': case 'S':
        observerVelocity -= 0.01f;
        if (observerVelocity < -0.9f) observerVelocity = -0.9f;
        updateDopplerShifts();
        break;
    case 'a': case 'A':
        viewAngle -= 5.0f;
        break;
    case 'd': case 'D':
        viewAngle += 5.0f;
        break;
    case 'k': case 'K':
        showKeplerian = !showKeplerian;
        break;
    case 'f': case 'F':
        showFlatRotation = !showFlatRotation;
        break;
    case 'r': case 'R':
        // Reset
        viewAngle = 0.0f;
        observerVelocity = 0.0f;
        showKeplerian = true;
        showFlatRotation = true;
        updateDopplerShifts();
        break;
    case 27:  // ESC key
        exit(0);
        break;
    }

    glutPostRedisplay();
}

// Reshape function
void reshape(int w, int h) {
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
}

// Idle function for continuous rotation
void idle() {
    // Automatic slow rotation
    viewAngle += 0.1f;
    if (viewAngle > 360.0f) viewAngle -= 360.0f;

    glutPostRedisplay();
}

// Main function
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Relativistic Doppler Effect: Galaxy Rotation Models");

    glClearColor(0.0f, 0.0f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

    // Initialize stars
    initializeStars();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle);

    std::cout << "Controls:" << std::endl;
    std::cout << "  W/S: Increase/decrease observer velocity" << std::endl;
    std::cout << "  A/D: Rotate view left/right" << std::endl;
    std::cout << "  K: Toggle Keplerian model display" << std::endl;
    std::cout << "  F: Toggle Flat rotation model display" << std::endl;
    std::cout << "  R: Reset view and settings" << std::endl;
    std::cout << "  ESC: Exit" << std::endl;

    glutMainLoop();
    return 0;
}