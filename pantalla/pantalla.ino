#include <Arduino.h>
#include <MCUFRIEND_kbv.h> //libreria de la pantalla
#include <Adafruit_GFX.h>
//#include <SPI.h>
//pin definiciones
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

//colores guardados
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GREY    0x778899
#define ORANGE  0xFF9900
#define PINK    0xFF66CC3
#define TURQUOISE 0x00e5ee


MCUFRIEND_kbv tft;// = MCUFRIEND_kbv(LCD_CS, LCD_CD, LCD_WR, LCD_CS, LCD_RESET); //la pantallita sera tft
int16_t g_identifier;

int game[32][18]; //matriz de piezas (paradas)

int posX, posY; //variables para saber pixel x e y
int fila,columna; //para saber fila y columna de la matriz que corresponde en la matriz

//variables para el joystick
int xPin = A5; //EJE x
int yPin = A4; //EJE Y
int buttonPin = 0; //Apretar botón

int xPosition = 0;
int yPosition = 0;
int buttonState = 1;

//pantalla inicial que muestra mensaje "Tetris"
void pantallaInicio() {
  tft.setCursor(50, 150); //nos ponemos en el pixel 50x150
  tft.setTextSize(4); //el texto sera de tamaño 4
  tft.setTextColor(GREEN);
  tft.println("Tetris!");
  delay(2000);
  tft.fillScreen(BLACK); //pone pantalla en negro
}

//degradado de fonde de pantalla
void dibujoInicial() {
  int LCD_HEIGHT = 320;
  int LCD_WIDTH = 240;
  int c = LCD_HEIGHT / 29;
  for (int i = LCD_HEIGHT - 1; i >= 0; i -= 2)
  {
    tft.drawRect(0, i, LCD_WIDTH, 2, 0x07ff - i / c);
  }
}

//crear bordes con unos y matriz vacia con ceros
void inicicializarMatrizGame() {
  for (int fil = 0; fil < 32; fil++) {
    for (int col = 0; col < 18; col++) {
      if (fil == 31 || col == 0 || col == 17) {
        game[fil][col] = 1;
      } else {
        game[fil][col] = 0;
      }
    }
  }
}

//elimina el contenido de la parte de la pantalla en la que se mueven las piezas
void reiniciarPantalla() {
  for (int i = 0; i < 5; i++) {
        tft.drawFastHLine(70, i, 160, CYAN);
  }
  for (int i = 6; i < 310; i++) {
        tft.drawFastHLine(70, i, 160, BLACK);
  }
}

//dibuja la parte estatica del Tetris
void dibujarPantalla() {
  //cuadrado en donde se moveran las piezas
  for (int i = 5; i < 310; i++) {
    tft.drawPixel(69, i, YELLOW);
    tft.drawPixel(230, i, YELLOW);
  }
  for (int i = 69; i < 230; i++) {
    tft.drawPixel(i, 5, YELLOW);
    tft.drawPixel(i, 310, YELLOW);
  }
  //pantalla pequeña siguiente pieza
  for (int i = 5; i < 56; i++) {
    tft.drawPixel(5, i, YELLOW);
    tft.drawPixel(55, i, YELLOW);
  }
  for (int i = 5; i < 56; i++) {
    tft.drawPixel(i, 5, YELLOW);
    tft.drawPixel(i, 55, YELLOW);
  }
}

//funcion que imprime una pieza --> LAS PIEZAS SON MATRICES DE 4*4
void draw(int pieza[4][4], int x, int y, int color) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (pieza[i][j]) { //si en esa posicion hay un 1 -->
        for (int c = 0; c < 10; c++) { //pintar cuadrado de 10x10
          for (int k = 0; k < 10; k++) {
            tft.drawPixel(x + 10 * j + c, y + 10 * (i - 1) - 5 + k , color); //para pintar en el lugar correcto que nos dice x y teniendo en cuenta la posicion en el array (x10)
          }
        }
      }
    }
  }
}

//giro 90 grados de la pieza
void rotatePieza(int origen[4][4], int destino[4][4]) {
  for (int i = 0; i < 4; i++) {
    destino[0][i] = origen[i][2];
    destino[1][i] = origen[i][1];
    destino[2][i] = origen[i][0];
    destino[3][i] = origen[i][3];
  }
}

//funcion que mira si la pieza es un cuadrado (para no rotar la pieza)
int esCuadrado(int pieza[4][4]) {
  int es = 1;
  int cuadrado[4][4] = {
    {1, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}
  };
  for (int i = 0; i < 4 && es; i++) {
    for (int j = 0; j < 2 && es; j++) {
      if (pieza[i][j] != cuadrado[i][j]) {
        es = 0;
      }
    }
    tft.println();
  }
  return es;
}

//copia la pieza de origen a destino
void copiarPieza(int origen[4][4], int destino[4][4]) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      destino[i][j] = origen[i][j];
    }
  }
}

//guarda la pieza (que ha caido) en la matriz game
void guardarPieza(int pieza[4][4], int fil, int col) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (pieza[i][j]) {
        game[fil + i][col + j] = 1;
      }
    }
  }
}

//[para depuracion] imprime la matriz de la pieza (para mostrar la matriz de ceros y unos)
void printPieza(int pieza[4][4]) {
  for(int i = 0; i < 4; i++) {
    for(int j = 0; j < 4; j++) {
      tft.print(pieza[i][j]);
    }
    tft.println("");
  }
}

//[para depuracion]pinta matriz de ceros y unos de game
void printGame() {
  tft.setCursor(0, 0);
  for (int fil = 0; fil < 32; fil++) {
    for (int col = 0; col < 18; col++) {
      tft.setTextSize(1);
      tft.print(game[fil][col]);
    }
    tft.println();
  }
}

//pinta las piezas que se hayan guardado en la matriz, es decir que hayan caido ya
void drawGame() {
  reiniciarPantalla();
  //dibujarPantalla();
  for (int i = 0; i < 32; i++) {
    for (int j = 0; j < 18; j++) {
      if ((i != 31) && (j != 17) && (j != 0))  { //para no pintar las paredes
        if (game[i][j]) { //si hay pieza --> pintarla
          for (int c = 0; c < 10; c++) { //pintar cuadrado de 10x10
            for (int k = 0; k < 10; k++) {
              tft.drawPixel(70 + 10 * (j - 1) + c, 10 * i + k, GREY);
            }
          }
        }
      }
    }
  }
}

//funcion que calcula cuando una pieza toca el suelo/otra pieza de la matriz
int tocaSuelo(int pieza[4][4], int fil, int col, int n) {
  for (int i = 0; i < 4; i++) { //para ver las diferentes columnas
    if (pieza[n][i]) { //la pieza tiene un uno en esa columna
      if (game[fil + 1][col + i]) { //debajo hay un uno en la matriz
        return 1; //si toca
      }
    }
  }
  return 0;
}

//funcion que mueve la pieza una posicion a la derecha
void moverDerecha(int pieza[4][4], int fil, int col) {
    for (int i = 3; i >= 0; i--) {
      if (tocaDerecha(pieza, fil, col, i)) { //si toca no mover
        return;
      }
    }
    //si no toca a la derecha --> mover
    columna = col + 1;
    posX = posX + 10;
}

//ver si una pieza toca la derecha
int tocaDerecha(int pieza[4][4], int fil, int col, int n) {
  for (int i = 0; i < 4 ; i++) {
    if (pieza[n][i]) { //la pieza tiene un uno en esa fila
      if (game[fil + n][col + i + 1]) { //a la derecha hay un uno en la matriz
        return 1;
      }
    }
  }
  return 0;
}

//funcion que mueve la pieza una posicion a la izquierda
void moverIzquierda(int pieza[4][4], int fil, int col) {
    for (int i = 0; i < 3; i++) {
      if (tocaIzquierda(pieza, fil, col, i)) {
        return;
      }
    }
    //si no toca a la izq --> mover
    columna = col - 1;
    posX = posX - 10;
}

//ver si una pieza toca la izquierda
int tocaIzquierda(int pieza[4][4], int fil, int col, int n) {
  for (int i = 0; i < 4 ; i++) {
    if (pieza[n][i]) { //la pieza tiene un uno en esa fila
      if (game[fil + n][col + i - 1]) { //a la izquierda hay un uno en la matriz
        return 1;
      }
    }
  }
  return 0;
}

//funcion que comprueba si no se chocaria al rotar pieza (la rota pero no la cambia por si no pudiera)
int verSiPuedeRotar(int pieza[4][4], int fil, int col) {
  int npieza[4][4];
  copiarPieza(pieza, npieza);
  rotatePieza(npieza, npieza);
  for(int i = 0; i<4; i++) {
    if ((tocaDerecha(npieza, fil, col, i)) || (tocaIzquierda(npieza, fil, col, i)) || (tocaSuelo(npieza, fil, col, i))) {
      return 0; //no se podría rotar
    }
  }
  return 1;
}

//funcion que mira si hay alguna fila de unos en la matriz --> eliminar esa fila
void eliminarFilas() {
  for (int i = 1; i < 31; i++) {
    int cont = 0;
    int hayCero = 0;
    for (int j = 1; (j < 17 && (!hayCero)); j++) {
      if (game[i][j] == 0) {
        hayCero = 1;
      }
      else {
        cont++;
      }

    }
    if (cont == 16) {
      for (int k = i; k > 0; k--) {
        for (int l = 1; l < 17; l++) {
          game[k][l] = game[k - 1][l];
        }
      }
    }
  }
}

//si hay una pieza en la primera fila de la matriz --> se acaba el juego
int finPartida() {
  for(int i = 1; i < 17; i++) {
    if (game[1][i]) {
      return 1;
    }
  }
  return 0;
}

//Mensaje de finalizacion del juego
void mostrarMensajeFin() {
  tft.fillScreen(BLACK);
  tft.setCursor(10, 150); //nos ponemos en el pixel 50x150
  tft.setTextColor(RED);
  tft.setTextSize(3); //el texto sera de tamaño 4
  tft.println("Perdiste :(");
  delay(5000);
}

//funcion que muestra la siguiente pieza que entrara en el tetris
void mostrarPiezaDespues(int pieza[4][4], int color) {
  for (int i = 6; i < 55; i++) {
    for (int j = 6; j < 55; j++) {
      tft.drawPixel(i, j, BLACK);
    }
  }
  draw(pieza, 25, 25, color);
}

//funcion que inicia la bajada de piezas --> empieza el juego!
void inicioJuego() {
  //matriz de piezas del tetris
  int piezas[7][4][4] = {
    {{1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}},
    {{1, 0, 0, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}},
    {{1, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
    {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}},
    {{0, 1, 0, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}},
    {{1, 0, 0, 0}, {1, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}}
  };

  //matriz de colores de las piezas
  int colores[7] = {
      PINK,
      BLUE,
      GREEN,
      MAGENTA,
      YELLOW,
      ORANGE,
      TURQUOISE
  };

  //selecciono las piezas para que salgan
  int npieza[4][4];
  int pieza[4][4];

  int randomPiezaDespues = random()%7;
  int randomPiezaAct;
  //bucle que va sacando hasta 100 piezas para jugar
  for(int c = 0; c<100; c++) {
     //elige una pieza al azar
    randomPiezaAct = randomPiezaDespues;
    randomPiezaDespues = (random()+3)%7;

    mostrarPiezaDespues(piezas[randomPiezaDespues], colores[randomPiezaDespues]);

    int cont = 1; //contador de movimiento de fila
    int toca = 0; //para saber si ha llegado abajo o toca alguna otra pieza
    int pieza[4][4];
    int npieza[4][4];
    copiarPieza(piezas[randomPiezaAct],pieza);
    posX = 150; //pixel x del que sale
    columna = 9; //columna que corresponde a la matriz

    //bucle que hace que piezas vayan bajando por la pantalla
    for (int i = 5; (i<tft.height()-24) && (!toca); i+=10) {
      posY = i;
      draw(pieza,posX,posY,colores[randomPiezaAct]); //pintar la pieza

      //mirar si toca el suelo/otra pieza
      for (int j = 3; (j>=0) && (!toca); j--) {
        if (tocaSuelo(pieza,cont,columna,j)) {
          toca = 1; //si toca suelo-->se para
          guardarPieza(pieza,cont-j,columna);
        }
      }
      //lectura de x e y del joystick
      xPosition = analogRead(xPin);
      yPosition = analogRead(yPin);
      //si pasa ciertos valores --> mover a izq/dcha o abajo
      if (xPosition < 500) {
        moverIzquierda(pieza, cont, columna);
        xPosition = analogRead(xPin);
        if (xPosition < 200) { //si sigue a la izq --> mover dos veces (para que vaya rapido)
          moverIzquierda(pieza, cont, columna);
        }
      } else  if (xPosition > 600) {
        moverDerecha(pieza, cont, columna);
        xPosition = analogRead(xPin);
        if (xPosition > 900) {
          moverDerecha(pieza, cont, columna);
        }
      }
      //mover para abajo
      if (yPosition > 600) {
        //primero ver si tocaria el suelo al mover una posicion abajo
        for (int j = 3; (j>=0) && (!toca); j--) {
          if (tocaSuelo(pieza,cont+1,columna,j)) {
            toca = 1; //si toca suelo-->se para
            guardarPieza(pieza,cont-j+1,columna);
          }
        }
        if (!toca) { //si no toca se puede bajar
          cont++;
          i = i + 10;
          fila++;
        }
      }
      //ver si hemos pulsado el boton de rotar la pieza
      buttonState = digitalRead(buttonPin);
      if (!buttonState && !esCuadrado(pieza)) {
        if (verSiPuedeRotar(pieza, fila, columna)) {
          rotatePieza(pieza, npieza);
          copiarPieza(npieza, pieza);
          buttonState = 1;
        }
      }
      delay(150); //delay de 250 ms
      //volvemos a mirar la rotacion
      cont++; //actualizar contador de filas
      eliminarFilas(); //por si hubiera una fila completa --> eliminarla
      //por si se hubiera acabado el juego
      if (finPartida()) {
        mostrarMensajeFin();
        return;
      }
      //pintar la matriz de las piezas por pantalla
      drawGame();
    }
  }
}


void setup() {
  //algunas configuraciones de la pantalla / joystick inicales
  g_identifier = tft.readID();
  //Serial.print("ID = 0x");
  //Serial.println(g_identifier, HEX);
  tft.begin(g_identifier);
  tft.setRotation(0); //pantalla en vertical

  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);
  //activar resistencia pull-up en el pin pulsador
  pinMode(buttonPin, INPUT_PULLUP);
}

void loop() { //bucle del juego
    tft.fillScreen(BLACK); //pone pantalla en negro
    pantallaInicio();
    dibujarPantalla();
    inicicializarMatrizGame();
    dibujoInicial();
    inicioJuego();
}
