/*
Niniejszy program jest wolnym oprogramowaniem; możesz go
rozprowadzać dalej i / lub modyfikować na warunkach Powszechnej
Licencji Publicznej GNU, wydanej przez Fundację Wolnego
Oprogramowania - według wersji 2 tej Licencji lub(według twojego
wyboru) którejś z późniejszych wersji.

Niniejszy program rozpowszechniany jest z nadzieją, iż będzie on
użyteczny - jednak BEZ JAKIEJKOLWIEK GWARANCJI, nawet domyślnej
gwarancji PRZYDATNOŚCI HANDLOWEJ albo PRZYDATNOŚCI DO OKREŚLONYCH
ZASTOSOWAŃ.W celu uzyskania bliższych informacji sięgnij do
Powszechnej Licencji Publicznej GNU.

Z pewnością wraz z niniejszym programem otrzymałeś też egzemplarz
Powszechnej Licencji Publicznej GNU(GNU General Public License);
jeśli nie - napisz do Free Software Foundation, Inc., 59 Temple
Place, Fifth Floor, Boston, MA  02110 - 1301  USA
*/

#define GLM_FORCE_RADIANS

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include "constants.h"
#include "allmodels.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <cstdlib> 
#include <ctime>

float speed_x = 0;//[radiany/s]
float speed_y = 0;//[radiany/s]
float aspectRatio = 1;
GLuint tex;


class Object3D {
public:
	// pola w obiekcie importu modelu 
	std::vector<glm::vec4> verts;
	std::vector<glm::vec4> norms;
	std::vector<glm::vec2> texCoords;
	std::vector<unsigned int> indices;
	GLuint tex;

	//konstruktor z wczytaniem danego obiektu
	Object3D(std::string plik, GLuint texture) {
		loadModel(plik);
		this->tex = texture;
		//readTexture(/*filename*/);
	}

	//dekonstrutor
	~Object3D() {}

	void loadModel(std::string plik) {

		using namespace std;

		Assimp::Importer importer;

		const aiScene* scene = importer.ReadFile(plik, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);
		cout << importer.GetErrorString() << endl;


		aiMesh* mesh = scene->mMeshes[0];

		for (int i = 0; i < mesh->mNumVertices; i++) {
			aiVector3D vertex = mesh->mVertices[i]; //aiVector3D podobny do glm::vec3
			this->verts.push_back(glm::vec4(vertex.x, vertex.y, vertex.z, 1));

			aiVector3D normal = mesh->mNormals[i]; //Wektory znormalizowane
			this->norms.push_back(glm::vec4(normal.x, normal.y, normal.z, 0));

			aiVector3D texCoord = mesh->mTextureCoords[0][i];
			this->texCoords.push_back(glm::vec2(texCoord.x, texCoord.y));

		}

		//dla każdego wielokąta składowego
		for (int i = 0; i < mesh->mNumFaces; i++) {
			aiFace& face = mesh->mFaces[i]; //face to jeden z wielokątów siatki

			//dla każdego indeksu->wierzchołka tworzącego wielokąt
			//dla aiProcess_Triangulate to zawsze będzie 3
			for (int j = 0; j < face.mNumIndices; j++) {
				this->indices.push_back(face.mIndices[j]);
			}

		}

		//aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		//for (int i = 0; i < 19; i++) {
		//	cout << i << " " << material->GetTextureCount((aiTextureType)i) << endl;
		//}

		//for (int i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++) {
		//	aiString str; //nazwa pliku
		//	material->GetTexture(aiTextureType_DIFFUSE, i, &str);
		//	cout << str.C_Str() << endl;
		//}
	}

	
};

GLuint readTexture(const char* filename) {
	GLuint texture;
	glActiveTexture(GL_TEXTURE0);

	std::vector<unsigned char> image;
	unsigned width, height;
	unsigned error = lodepng::decode(image, width, height, filename);

	glGenTextures(1, &(texture));
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return texture;
}

Object3D* cloud;
Object3D* wulkan;

std::vector<glm::vec4> verts;
std::vector<glm::vec4> norms;
std::vector<glm::vec2> texCoords;
std::vector<unsigned int> indices;

//Procedura obsługi błędów
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

void key_callback(
	GLFWwindow* window,
	int key,
	int scancode,
	int action,
	int mod
) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_LEFT) {
			speed_y = PI;
		}
		if (key == GLFW_KEY_RIGHT) {
			speed_y = -PI;
		}
		if (key == GLFW_KEY_UP) {
			speed_x = -PI;
		}
		if (key == GLFW_KEY_DOWN) {
			speed_x = PI;
		}
	}
	if (action == GLFW_RELEASE) {
		if (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT) {
			speed_y = 0;
		}
		if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN) {
			speed_x = 0;
		}
	}
}



struct particle {
	glm::vec3 position; //położenie cząstki
	glm::vec3 speed; //prędkość cząstki
	float ttl; //czas życia
};

float normaldistr() {
	srand((unsigned)time(NULL));
	return (float)rand() / RAND_MAX;
}

const int n = 1000; //liczba cząstek
particle system1[n]; //tablica cząstek
glm::vec3 gravity = glm::vec3(0, -1, 0); //wektor grawitacji
void createparticle(particle& p) { //zainicjowanie cząstki
	p.position = glm::vec3(0, 0, 0);
	p.speed = glm::vec3(normaldistr(), 5 * abs(normaldistr()), normaldistr());
	p.ttl = 10;
}

void initializesystem(particle* system1, int n) {//zainicjowanie każdej cząstki
	for (int i = 0; i < n; i++) createparticle(system1[i]);
}
void processsystem(particle* system1, glm::vec3 gravity, int n, float timestep) {
	for (int i = 0; i < n; i++) {
		system1[i].position += system1[i].speed * timestep; //przesunięcie
		system1[i].speed += gravity * timestep; //uwzględnienie grawitacji
		system1[i].ttl -= timestep; //skrócenie czasu życia cząstki
		if (system1[i].ttl <= 0) createparticle(system1[i]);//reinkarnacja cząstki
	}
}

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	if (height == 0) return;
	aspectRatio = (float)width / (float)height;
	glViewport(0, 0, width, height);
}

//Procedura inicjująca
void initOpenGLProgram(GLFWwindow* window) {
    initShaders();
	//glClearColor(0, 0, 0, 1); //Ustaw kolor czyszczenia bufora kolorów
	glClearColor(0.533, 0.921, 0.964, 1);
	glEnable(GL_DEPTH_TEST); //Włącz test głębokości na pikselach
	glfwSetKeyCallback(window, key_callback);
	glfwSetWindowSizeCallback(window, windowResizeCallback);
	GLuint wulkanTex = readTexture("pngegg.png");
	GLuint cloudTex = readTexture("smoke.png");
	wulkan = new Object3D(std::string("cube.obj"), wulkanTex);
	cloud = new Object3D(std::string("cloud.obj"), cloudTex);
	initializesystem(system1, n);
}


//Zwolnienie zasobów zajętych przez program
void freeOpenGLProgram(GLFWwindow* window) {
    freeShaders();
	glDeleteTextures(1, &(wulkan->tex));
	glDeleteTextures(1, &(cloud->tex));
	delete wulkan;
	delete cloud;
}

void createObject(Object3D* object, glm::mat4 P, glm::mat4 V, glm::mat4 M) {
	spLambertTextured->use();
	glUniformMatrix4fv(spLambertTextured->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(spLambertTextured->u("V"), 1, false, glm::value_ptr(V));
	glUniformMatrix4fv(spLambertTextured->u("M"), 1, false, glm::value_ptr(M));

	glEnableVertexAttribArray(spLambertTextured->a("vertex"));
	glVertexAttribPointer(spLambertTextured->a("vertex"), 4, GL_FLOAT, false, 0, object->verts.data());
	
	glEnableVertexAttribArray(spLambertTextured->a("normal"));
	glVertexAttribPointer(spLambertTextured->a("normal"), 4, GL_FLOAT, false, 0, object->norms.data());

	glEnableVertexAttribArray(spLambertTextured->a("texCoord"));
	glVertexAttribPointer(spLambertTextured->a("texCoord"), 2, GL_FLOAT, false, 0, object->texCoords.data());

	glUniform1i(spLambertTextured->u("tex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, object->tex);


	glDrawElements(GL_TRIANGLES, object->indices.size(), GL_UNSIGNED_INT, object->indices.data());

	glDisableVertexAttribArray(spLambertTextured->a("vertex"));
	glDisableVertexAttribArray(spLambertTextured->a("texCoord"));
	glDisableVertexAttribArray(spLambertTextured->a("normal"));

}



//Procedura rysująca zawartość sceny
void drawScene(GLFWwindow* window,float angle_x,float angle_y) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Wyczyść bufor koloru i bufor głębokości

	glm::mat4 V = glm::lookAt(glm::vec3(20.0f, 13.0f, 10.0f), glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f)); //Wylicz macierz widoku
	glm::mat4 P = glm::perspective(50.0f * PI / 180.0f, aspectRatio, 1.0f, 150.0f); //Wylicz macierz rzutowania

	glm::mat4 M_wulkan = glm::mat4(1.0f); //Zainicjuj macierz modelu macierzą jednostkową
	glm::mat4 M_Cloud = glm::mat4(1.0f); //Zainicjuj macierz modelu macierzą jednostkową

	M_Cloud = glm::rotate(M_Cloud, angle_x * PI / 180, glm::vec3(1.0f, 0.0f, 0.0f));
	M_wulkan = glm::rotate(M_wulkan, angle_y * PI / 180, glm::vec3(1.0f, 0.0f, 0.0f));

	M_Cloud = glm::scale(M_Cloud, glm::vec3(0.01f, 0.01f, 0.01f));
	M_wulkan = glm::scale(M_wulkan, glm::vec3(0.01f, 0.01f, 0.01f));

	// pozostałe zabawy z wektorami, czyli rotate, translate, scale

	//SYSTEM CZĄSTECZEK
	//for (int i = 0; i < n; i++) {
	//	glm::mat4 M_cloud = M_wulkan;
	//	//zabawa na wektorach jak wyżej
	//	createObject(cloud, M_cloud, V, P);
	//}
	createObject(wulkan, M_wulkan, V, P);
	//createObject(cloud, M_Cloud, V, P);
	
	processsystem(system1, gravity, n, 0.01);

	glfwSwapBuffers(window); //Skopiuj bufor tylny do bufora przedniego
}



int main(void)
{
	GLFWwindow* window; //Wskaźnik na obiekt reprezentujący okno

	/*while (!glfwWindowShouldClose(window)) {
		angle_x += speed_x * glfwGetTime();
		angle_y += speed_x * glfwGetTime();
		glfwSetTime(0);
		drawScene(window, angle_x, angle_y);
		glfwPollEvents();
	}*/

	glfwSetErrorCallback(error_callback);//Zarejestruj procedurę obsługi błędów

	if (!glfwInit()) { //Zainicjuj bibliotekę GLFW
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(500, 500, "OpenGL", NULL, NULL);  //Utwórz okno 500x500 o tytule "OpenGL" i kontekst OpenGL.

	if (!window) //Jeżeli okna nie udało się utworzyć, to zamknij program
	{
		fprintf(stderr, "Nie można utworzyć okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window); //Od tego momentu kontekst okna staje się aktywny i polecenia OpenGL będą dotyczyć właśnie jego.
	glfwSwapInterval(1); //Czekaj na 1 powrót plamki przed pokazaniem ukrytego bufora

	if (glewInit() != GLEW_OK) { //Zainicjuj bibliotekę GLEW
		fprintf(stderr, "Nie można zainicjować GLEW.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window); //Operacje inicjujące

	//Główna pętla
	float angle_x = 0;
	float angle_y = 0;
	float speed_x = 0; //zadeklaruj zmienną przechowującą aktualny kąt obrotu
	float speed_y = 0; //zadeklaruj zmienną przechowującą aktualny kąt obrotu
	glfwSetTime(0); //Wyzeruj licznik czasu
	while (!glfwWindowShouldClose(window)) //Tak długo jak okno nie powinno zostać zamknięte
	{
		angle_x += speed_x * glfwGetTime(); //Oblicz kąt o jaki obiekt obrócił się podczas poprzedniej klatki
		angle_y += speed_y * glfwGetTime(); //Oblicz kąt o jaki obiekt obrócił się podczas poprzedniej klatki
		glfwSetTime(0); //Wyzeruj licznik czasu
		drawScene(window,angle_x,angle_y); //Wykonaj procedurę rysującą
		glfwPollEvents(); //Wykonaj procedury callback w zalezności od zdarzeń jakie zaszły.
	}

	
	

	freeOpenGLProgram(window);

	glfwDestroyWindow(window); //Usuń kontekst OpenGL i okno
	glfwTerminate(); //Zwolnij zasoby zajęte przez GLFW
	exit(EXIT_SUCCESS);
}
