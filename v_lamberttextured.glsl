#version 330

//Zmienne jednorodne
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;


uniform vec4 lightDir=vec4(0,0,1,0);

//Atrybuty
in vec4 vertex; //wspolrzedne wierzcholka w przestrzeni modelu
in vec4 normal; //wektor normalny w wierzcholku
in vec2 texCoord; //wspó³rzêdne teksturowania


//Zmienne interpolowane
out vec2 i_tc;
out float i_nl;

void main(void) {
    gl_Position=P*V*M*vertex;

    mat4 G=mat4(inverse(transpose(mat3(M))));
    vec4 n=normalize(V*G*normal);

    i_nl=clamp(dot(n,lightDir),0,1);
    i_tc=texCoord;
}
