#include <shell.h>
#include <hangman.h>
#include <userstdlib.h>
#include <sudoku.h>

#define COMMANDS_QTY 7
#define TICKS_PER_SECOND 18

#define SUDOKU_SCREEN 0
#define CHRONO_SCREEN 1
#define HANGMAN_SCREEN 2
#define TIME_SCREEN 3

typedef void (*void_function)(void);

typedef struct{
  void_function action;
  char * name;
}command;

static void help(void);
static void time(void);
static void date(void);
static void play(void);
static void divide_by_zero(void);
static void invalid_operation(void);
static void print_mem(void);

static command valid_commands[COMMANDS_QTY] = {{&help,"help"}, {&time,"time"}, {&date,"date"},{&play, "play"},{&divide_by_zero,"divide"},{&invalid_operation,"invalid"}, {&print_mem, "printmem"}};

static uint8_t modify_chrono(char * chrono, uint8_t ms_ticks);
static void restart(char * chrono);
static int execute_command(char * command);

void wait_command(void){

  char c = 0;
	char command[128];
	int i = 0;

  // Read the command until the user presses enter
  while((c=get_char())!='\n'){
		put_char(1,c);
		command[i++] = c;
	}
  sprint(2, "\n");
	command[i] = 0;

  //Check if de command is valid.
  // Execute it if its valid
  if(!execute_command(command)){
    //If not valid, show mensage
    sprint(2, "Invalid command \n");
    sprint(2, command);
    sprint(2, "\n");
  }
}

int execute_command(char * command){
  for(int i=0 ; i<COMMANDS_QTY ; ++i){
    if(strcmp(command,valid_commands[i].name) == 0){
      valid_commands[i].action();
      return 1;
    }
  }
  return 0;
}

void divide_by_zero(void){
  divideByZero();
}

void invalid_operation(void){
  invalidOp();
}

void print_mem(void){
  printMem();
}

void help(void){
  sprint(1, "el unico comando valido por ahora es help \n");

  /*
    //clear
	  //sprint(1, "'clear' - Comando para limpiar la pantalla");
    //inforeg
    sprint(1, "'inforeg' - Comando para imprime todos los registros.");
    //printmem c
    sprint(1, "'printmem' - Comando para realizar un volcado de memoria de 32 bytes a partir del puntero pasado como parametro ");
    //time
	  sprint(1, "'time' - Comando utilizado para desplegar el dia y hora del sistema");

    //cronometro

    //sudoku

    //exit
	  sprint(1, "'exit' - Comando para apagar el sistema");
    */
}

void time(void){
  char time[11];
  get_time(time);
  sprint(1, "Hora: ");
  sprint(1, time);
  sprint(2, "\n");
}

void date(void){
  char date[11];
  get_date(date);
  sprint(1, "Fecha: ");
  sprint(1, date);
  sprint(2, "\n");
}



void handle_chrono(int tick, int c){
  static uint8_t paused = 0;
  static uint8_t ms_ticks = 0;
  static char chrono[] = {'0',':','0','0',':','0','0',',','0',' ',' ',0};

  setScreen(CHRONO_SCREEN);

  if(!paused && tick){
    ms_ticks = modify_chrono(chrono,++ms_ticks);
    restartCursor();
    sprint(1, chrono);
  }
  if(c=='0'){
    restart(chrono);
    paused = 1;
    restartCursor();
    sprint(1, chrono);
  }
  else if(c == '+'){
    paused = !paused;
  }

}

void handle_time(int tick, int c){
  setScreen(TIME_SCREEN);
  static uint8_t tick_counter = 0;
  if(tick){
    if(!(tick_counter%18)){
      char time[11];
      get_time(time);
      restartCursor();
      sprint(1, time);
      tick_counter = 0;
    }
    ++tick_counter;
  }
}

void handle_sudoku(int tick, int c){
  setScreen(SUDOKU_SCREEN);
  restartCursor();
  // Si se presiono una flecha, movemos al usuario
  if(c>=37 && c<=40){
    moveUser(c);
  }
  // Si se presiono un numero, lo escribimos en el sudoku
  else if(c>='1' && c<='9'){
    writeNumber(c);
  }

  printSudoku();
}


void handle_hangman(int tick, int c){


  if(c>='a' && c<='z' || c>='A' && c<='Z'){
    setScreen(HANGMAN_SCREEN);
    check_letter(c);
    printHangman();
  }


}

void play(void){

  divideWindow();
  setScreen(SUDOKU_SCREEN);
  initSudoku();
  setScreen(HANGMAN_SCREEN);
  initHangman();

  int c;
  int t;
  uint8_t quit = 0;




  while(!quit){
    c = read_char();
    t= tick();
    handle_chrono(t, c);
    handle_time(t, c);
    handle_sudoku(t,c);
    handle_hangman(t,c);
    if(c== ' '){
      quit = 1;
    }
  }

  uniqueWindow();
}


uint8_t modify_chrono(char * chrono, uint8_t ms_ticks){

  if(ms_ticks == TICKS_PER_SECOND){
    chrono[8] = '0';
    if(chrono[6] == '9'){
      chrono[6] = '0';
      if(chrono[5] == '5'){
        chrono[5] = '0';
        if(chrono[3] == '9'){
          chrono[3] = '0';
          if(chrono[2] == '5'){
            chrono[2] = '0';
            chrono[0]++;
          }else{
            chrono[2]++;
          }
        }else{
          chrono[3]++;
        }
      }else{
        chrono[5]++;
      }
    }else{
      chrono[6]++;
    }
  }else{
    chrono[8] = '0' + (ms_ticks * 10)/18;
  }
  return ms_ticks%TICKS_PER_SECOND;
}

void restart(char * chrono){
  chrono[0] = chrono[2] = chrono[3] = '0';
  chrono[5] = chrono[6] = chrono[8] = '0';
}
