#include <EEPROM.h>
#include  "instruction_set.h"
#include  "test_programs.h"

//Deze is voor het poppen
char popStringArray[12] = "";

//Deze is voor de stack
char stackOutput[12];
int amoutBytes;
byte tempType;
int tempBeginPositieArray;
int tempBeginPositie;

typedef struct MyObject2 {
  byte naam;
  byte type;
  int adres;
  int grootte;
  int proces_ID;
}; MyObject2 memory_table[25];
byte geheugen[256];
int noOfVars = 0;

//Dit is voor de processen
int proces_ID_Counter = 0;

typedef struct MyObject1 {
  char naam[12];
  int proces_ID;
  char toestand;
  int programCounter; // Hiermee lees je de bytes uit!
  int file_pointer;   // Wijst waarnaar de files staan
  int stack_pointer; //Wordt gebruikt om door de stack heen te gaan
  int adres;      //Begin loop??
  byte stack[32]; //Dit is de stack waarmee je werkt
}; MyObject1 Proces_Table[10];

//Dit is voor het bestandSysteem
char received [12] = "";
char instruction[12] = "";
char variable1[12] = "";
char variable2[12] = "";
char variable3[12] = "";

int fase = 0;
int numbOfFiles = 0;
char readEEPROMarray[12];
byte readEEPROMarrayBYTE[12];

//De groote is 16 BYTES!!
typedef struct MyObject {
  char bestandsnaam[12];
  int beginpositie;
  int lengte;
}; MyObject customVar; MyObject customVar1; MyObject customVar2;

void setup() {
  Serial.begin(9600);
  Serial.println(F("Voer uw command in: "));  
  plaatsGroote();
  createProcesTable();
}

void loop() {
  clearReceived();
  readInput();
  delay(100);
  checkInput();
  runProcesses();
}

//////////////CheckPoint7: instructies uitvoeren////////////////////
void runProcesses() {
  for (int i = 0; i < 10; i++) {
    if (Proces_Table[i].toestand == 'r') {
      execute(i);
    }
  }
}

void execute(int indexProces) {
  char tempOpslaan[1] = "";
  char tempOpslaan2[1] = "";
  int temp;
  for (int i = 0; i < 160; i += 16) {
    EEPROM.get(i, customVar);
    if (strcmp(customVar.bestandsnaam, Proces_Table[indexProces].naam) == 0) {
      EEPROM.get(Proces_Table[indexProces].programCounter++, tempOpslaan);
      temp = atoi(tempOpslaan);
      Serial.println(temp);
      break;
    }
  }

  switch (temp) {
    case SUSPENDD:
      SUSPEND(indexProces);
      break;

    //Eerste naam dan grootte
    case STORE:
      Serial.println(F("lukt niet..."));
      break;


    case SET:
      Serial.println(F("SET!"));
      EEPROM.get(Proces_Table[indexProces].programCounter++, tempOpslaan);
      writeByte(tempOpslaan[0], indexProces);
      break;

    case INT:
      for (int i = 0; i < 2; i++) {
        EEPROM.get(Proces_Table[indexProces].programCounter++, tempOpslaan2);
        int temptemp = atoi(tempOpslaan2);
        pushByte(temptemp, indexProces);
        Serial.println(temptemp);
      }
      pushByte(INT, indexProces);
      break;

    case FLOAT:
      for (int i = 0; i < 4; i++) {
        EEPROM.get(Proces_Table[indexProces].programCounter++, tempOpslaan2);
        int temptemp = atoi(tempOpslaan2);
        Serial.println(temptemp);
        pushByte(temptemp, indexProces);
      }
      pushByte(FLOAT, indexProces);
      break;

    case CHAR:
      EEPROM.get(Proces_Table[indexProces].programCounter++, tempOpslaan2);
      pushByte(tempOpslaan2[0], indexProces);
      Serial.println(tempOpslaan2[0]);
      pushByte(CHAR, indexProces);
      break;

    case STRING:
      char a;
      int iteraties = 0;
      while (true) {
        EEPROM.get(Proces_Table[indexProces].programCounter++, a);
        pushByte(a, indexProces);
        Serial.println(a);
        if (a == '0') {
          pushByte(iteraties, indexProces);
          pushByte(STRING, indexProces);
          break;
        }
        iteraties++;
      } break;
  }
}
///////////////STACK////////////////
void pushByte(byte b, int i) {
  Proces_Table[i].stack[Proces_Table[i].stack_pointer++] = b;
}

byte popByte(int i) {
  return Proces_Table[i].stack[--Proces_Table[i].stack_pointer];
}

void clearStack(int i) {
  while (Proces_Table[i].stack_pointer > -1) {
    Serial.println(popByte(i));
  }
}

byte Peek(int proces) {
  byte temp = popByte(proces);
  pushByte(temp, proces);
  return temp;
}

float popVal(int proces) {
  byte temp = popByte(proces);
  if (temp == INT) {
    return popInt(proces);
  }
  if (temp == FLOAT) {
    return popFloat(proces);
  }

  if (temp == CHAR) {
    return popByte(proces);
  }
}

float popFloat(int proces) {
  byte b[4];
  float *pf = (float *)b;

  for (int i = 0; i < 4; i++) {
    b[i] = popByte(proces);
  }
  return *pf;
}

int popInt(int proces) {
  word LowByte = popByte(proces);
  word HighByte = popByte(proces);
  return LowByte + (HighByte * 256);
}

void popString(int proces) {
  int amoutChar = popByte(proces);
  for (int i = amoutChar - 1; i > -1; i--) {
    popStringArray[i] = popByte(proces); // popStringArray is een globale variable
  }
}

void pushString(char string[12], int proces) {
  for (int i = 0; i < 12; i++) {
    if (string[i] == '\0') {
      pushByte(i, proces);
      pushByte(STRING, proces);//Moet je eruit halen bij het testen!
      return;
    }
    pushByte(string[i], proces);
  }
}

void pushFloat(float kommaGetal, int proces) {
  byte b[4];
  float *pf = (float *)b;
  *pf = kommaGetal;

  for (int i = 3; i > -1; i--) {
    Serial.println(b[i]);
    pushByte(b[i], proces);
  }
  pushByte(FLOAT, proces); //Moet je weg laten voor testen
}

void pushInt(int getal, int proces) {
  pushByte(highByte(getal), proces);
  pushByte(lowByte(getal), proces);
  pushByte(INT, proces); // moet je weghalen voor testen
}

void VisualizeProcesTable() {
  for (int i = 0; i < 10; i++) {
    Serial.print(Proces_Table[i].proces_ID);
    Serial.print(' ');
    Serial.print(Proces_Table[i].toestand);
    Serial.print(' ');
    Serial.print(Proces_Table[i].naam);
    Serial.print(' ');
    Serial.print(Proces_Table[i].programCounter);
    Serial.println();
  }
}

void createProcesTable() {
  for (int i = 0; i < 10; i++) {
    strcpy(Proces_Table[i].naam, "empty");
  }
}

int SUSPEND(int proces_id) {
  for (int i = 0; i < 10; i++) {
    if ((proces_id == Proces_Table[i].proces_ID) and (Proces_Table[i].toestand == 'r' or Proces_Table[i].toestand == 'p')) {
      AndereToestand(i, 'p');
      return i;
    }
  }
}

int RESUME(int proces_id) {
  for (int i = 0; i < 10; i++) {
    if ((strcmp(Proces_Table[i].toestand, 'r') == 0 or strcmp(Proces_Table[i].toestand, 'p') == 0) and Proces_Table[i].proces_ID == proces_id) {
      AndereToestand(i, 'r');
      return i ;
    }
  }
}

int KILL(int proces_id) {
  for (int i = 0; i < 10; i++) {
    if ((strcmp(Proces_Table[i].toestand, 'r') == 0 or strcmp(Proces_Table[i].toestand, 'p') == 0) and Proces_Table[i].proces_ID == proces_id) {
      //strcpy(Proces_Table[i].naam, "empty");
      Proces_Table[i].proces_ID = 0;
      AndereToestand(i, '0');
      return i ;
    }
  }
}

void AndereToestand(int index, char gewensteToestand) {
  if (Proces_Table[index].toestand == gewensteToestand) {
  }
  else {
    Proces_Table[index].toestand = gewensteToestand;
    Serial.println(gewensteToestand);
  }
}

void RUN(char Naam[12]) {
  for (int i = 0; i < 10; i++) {
    if (strcmp(Proces_Table[i].naam, "empty") == 0) {
      for (int j = 0; j < 160; j += 16) {
        EEPROM.get(j, customVar);
        if (strcmp(Naam, customVar.bestandsnaam) == 0) {
          strcpy(Proces_Table[i].naam, Naam);
          Proces_Table[i].proces_ID = proces_ID_Counter++;
          Proces_Table[i].programCounter = customVar.beginpositie;
          Proces_Table[i].stack_pointer = 0;
          Proces_Table[i].toestand = 'r';
          Serial.print(F("Proces is gestart")); Serial.print(' '); Serial.print(Proces_Table[i].proces_ID); Serial.println();
          return;
        }
      }
    }
  }
}

void ERASE(char naam[12]) {
  for (int i = 0; i < 160; i += 16) {
    EEPROM.get(i, customVar);
    if (strcmp(naam , customVar.bestandsnaam) == 0) {
      for (int j = customVar.beginpositie; j <= customVar.lengte; j++) {
        EEPROM.write(j, 0);
      }
      MyObject temp = {"empty", 170, 0};
      EEPROM.put(i, temp);
      int c;
      EEPROM.get(161, c);
      c--;
      EEPROM.put(161, c);
      break;
    }
  }
}
////////////////////////////OPDRACHT 3///////////////////////////
void readCharArrayEEPROM(int locatie, int groote) {
  for (int i = 0; i < groote; i++) {
    EEPROM.get(locatie, readEEPROMarray[i]);
    locatie++;;
  }
}

void readCharArrayEEPROMBYTE(int locatie, int groote) {
  for (int i = 0; i < groote; i++) {
    EEPROM.get(locatie, readEEPROMarrayBYTE[i]);
    locatie++;;
  }
}

void writeCharArrayToEEPROM(char input[12], int locatie) {
  int tempInt = locatie;
  for (int i = 0; i < 12; i++) {
    if (input[i] == '\0') {
      break;
    }
    else {
      EEPROM.put(tempInt, input[i]);
      tempInt++;
    }
  }
}

void visualizeFAT() {
  Serial.println("N | B | L");
  for (int i = 0; i < 160; i += 16) {
    EEPROM.get(i, customVar);
    Serial.print(i);
    Serial.print(F(" "));
    Serial.print(customVar.bestandsnaam);
    Serial.print(F(" "));
    Serial.print(customVar.beginpositie);
    Serial.print(F(" "));
    Serial.print(customVar.lengte);
    Serial.println();
  }
  int c;
  EEPROM.get(161, c);
  Serial.print(F("hoeveelheid bestand: ")); Serial.println(c);
}

void freespace() {
  int temp = 0;
  int beginpunt = 0;
  int eindpunt = 0;
  for (int i = 0; i < 160; i += 16) {
    EEPROM.get(i, customVar1);
    if (strcmp("empty", customVar1.bestandsnaam) == 0) {
      if (beginpunt == 0) {
        beginpunt = customVar1.beginpositie;
      }
      else if (customVar1.beginpositie > eindpunt) {
        if (customVar1.beginpositie == 170) {
          eindpunt = 1024;
        }
        else {
          eindpunt = customVar1.beginpositie;
        }
      }
    }

    else if (strcmp("empty", customVar1.bestandsnaam) != 0) {
      beginpunt = 0;
      eindpunt = 0;
    }

    if (eindpunt - beginpunt > temp) {
      temp = eindpunt - beginpunt;
    }
    //Serial.print(beginpunt); Serial.print(' '); Serial.print(eindpunt); Serial.println();
  }
  Serial.println(temp);
}

void retrieve(char naam[12]) {
  for (int i = 0; i < 160; i += 16) {
    EEPROM.get(i, customVar);
    if (strcmp(naam, customVar.bestandsnaam) == 0) {
      readCharArrayEEPROM(customVar.beginpositie, customVar.lengte);
      Serial.println(readEEPROMarray);
      for (int i = 0; i < 12; i++) {
        readEEPROMarray[i] = '\0';
      }
      return;
    }
  }
}

void retrieveBYTE(char naam[12]) {
  for (int i = 0; i < 160; i += 16) {
    EEPROM.get(i, customVar);
    if (strcmp(naam, customVar.bestandsnaam) == 0) {
      readCharArrayEEPROM(customVar.beginpositie, customVar.lengte);
      Serial.println(readEEPROMarray);
      for (int i = 0; i < 12; i++) {
        readEEPROMarrayBYTE[i] = '\0';
      }
      return;
    }
  }
}

//////////////////////////////////////MISSCHIEN EEN FUNCTIE MAKEN DIE HET TERUG SCHRIJFT!!//////////////////////////////
void STOREPROG(char prog[12]) {
  int index = 0;
  char grootte[12];
  char data[20] = "";
  if (strcmp(prog, "string") == 0) {
    index++;
    for (int i = 0; i < 20; i++) {
      data[i] = prog1[index++];
      if (data[i] == 0) {
        itoa(i, grootte, 10);
        store("STRING", grootte, data);
        Serial.print(grootte); Serial.print(' '); Serial.print(data);
        break;
      }
    }
  }
  if (strcmp(prog, "int") == 0) {
    data[0] = prog2[1];
    data[1] = prog2[2];
    store("INT", "2", data);
  }

  if (strcmp(prog, "float") == 0) {
    index = 1;
    for (int i = 0; i < 4; i++) {
      data[i] = prog3[index++];
    }
    store("FLOAT", "4", data);
  }
  if (strcmp(prog, "char") == 0) {
    data[0] = prog4[1];
    store("CHAR", "1", data);
  }
}

void store(char naam[12], char groote[12], char data[12]) {
  bool genoegruimte = false;
  bool naamBestaat = false;
  int beschikbareLocatie = 0;
  int FATlocatie = 0;

  //Hier kijk ik of de naam al bestaat.
  for (int i = 0; i < 160; i += 16) {
    EEPROM.get(i, customVar);
    if (strcmp(naam, customVar.bestandsnaam) == 0) {
      Serial.print(customVar.bestandsnaam);
      naamBestaat = true;
      Serial.print(F(" bestaat al in eepromplek "));
      Serial.println(customVar.beginpositie);
      break;
    }
  }

  //Hier kijk ik of er genoeg ruimte is!
  if (naamBestaat == false) {
    for (int i = 0; i < 160; i += 16) {
      EEPROM.get(i, customVar1);
      for (int j = i + 16; j < 160; j += 16) {
        EEPROM.get(j, customVar2);
        if (strcmp(customVar2.bestandsnaam, "empty") != 0) {
          int limit = customVar2.beginpositie;
          Serial.println(limit);
          Serial.println(customVar2.bestandsnaam);
          break;
        }
      }

      //EEPROM.get(i + 16, customVar2);
      int gebruikteBytes = customVar1.beginpositie + customVar1.lengte;
      int limit = customVar2.beginpositie;

      if (((limit - gebruikteBytes) > atoi(groote) || strcmp(customVar2.bestandsnaam , "empty") == 0)  && strcmp(customVar1.bestandsnaam , "empty") == 0) {
        Serial.print(F("genoeg ruimte gevonden!In de FAT van ")); Serial.print(i); Serial.print(F(" ")); Serial.print(limit); Serial.print(F(" ")); Serial.println(gebruikteBytes);
        Serial.println(naam);
        updateEEPROM(i, naam, customVar1.beginpositie, atoi(groote));

        writeCharArrayToEEPROM(data , customVar1.beginpositie);
        int c;
        EEPROM.get(161, c);
        c++;
        EEPROM.put(161, c);

        if (strcmp(customVar2.bestandsnaam , "empty") == 0) {
          MyObject temp = {"empty", customVar1.beginpositie + atoi(groote) , 0};
          EEPROM.put(i + 16, temp);
        }
        break;
      }
      else {
        Serial.println(F("Kon geen plek vinden")); Serial.print(i); Serial.print(" "); Serial.print(limit); Serial.print(" "); Serial.println(gebruikteBytes);
      }
    }
  }
  visualizeFAT();
}

void updateEEPROM(int locatie, char naamBestand[12], int positieBestand, int lengteBestand) {
  strcpy(customVar.bestandsnaam, naamBestand);
  customVar.beginpositie = positieBestand;
  customVar.lengte = lengteBestand;
  EEPROM.put(locatie, customVar);
}

//10 structs pushen (doet het goed!) laatste is op 144 + 16 = 160 Volgende beschikbare
void refreshFAT() {
  //clearEEPROM();
  MyObject temp = {"empty", 170, 0};
  for (int i = 0 ; i < 160; i += 16) {
    EEPROM.put(i, temp);
    Serial.println(i);
  }
  EEPROM.put(161, numbOfFiles);
}

void clearEEPROM() {
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  Serial.println(F("done"));
}

//Krijgt alle data uit EEPROM
void readFATEntry(int locatie) {
  EEPROM.get(locatie, customVar);
  Serial.print(customVar.bestandsnaam);
  Serial.print(customVar.beginpositie);
  Serial.print(customVar.lengte);
}
////////////////////////////OPDRACHT 2///////////////////////////
void readInput() {
  int i = 0;
  while (Serial.available() > 0)
  {
    received[i] = Serial.read();
    i++;
  }
}

void clearReceived() {
  for (int i = 0; i < sizeof(received); i++) {
    received[i] = "\0";
  }
}

void clearAllVariables() {
  for (int i = 0; i < 12; i++) {
    instruction[i] = "\0";
    variable1[i] = "\0";
    variable2[i] = "\0";
    variable3[i] = "\0";
  }
}

void checkInput() {
  //Basic van de funtie retrieve
  //Basic van de funtie retrieve
  if (strcmp(received, "retrieve") == 0 && fase == 0) {
    for (int i = 0; i < sizeof(received); i++) {
      instruction[i] = received[i];
    }
    Serial.print(F("bestand: "));
    fase = 4;
  }

  else if (strcmp(received, "files") == 0) {
    visualizeFAT();
  }

  else if (strcmp(received, "refresh") == 0) {
    refreshFAT();
    Serial.println(F("refresh done!"));
  }

  else if (strcmp(received, "memoryTable") == 0) {
    visualizeMemoryTabel();
  }

  else if (strcmp(received, "getVar") == 0) {
    Serial.println(F("CHAR: "));
    fase = 5;
  }

  else if (strcmp(received, "variable") == 0) {
    visualizeMemoryTabel();
  }

  //Basic van de functie Store
  else if (strcmp(received, "store") == 0 && fase == 0) {
    Serial.print(F("bestand: "));
    fase = 1;
  }

  else if (strcmp(received, "erase") == 0 && fase == 0) {
    Serial.print(F("bestand: "));
    for (int i = 0; i < sizeof(received); i++) {
      instruction[i] = received[i];
      fase = 4;
    }
  }

  else if (strcmp(received, "storeProg") == 0 && fase == 0) {
    Serial.println(F("prgramma: || string/int/char/float"));
    for (int i = 0; i < sizeof(received); i++) {
      instruction[i] = received[i];
      fase = 4;
    }
  }

  else if (strcmp(received, "files") == 0 && fase == 0) {
    for (int i = 0; i < sizeof(received); i++) {
      instruction[i] = received[i];
    }
  }

  else if (strcmp(received, "freespace") == 0 && fase == 0) {
    freespace();
  }

  else if (strcmp(received, "run") == 0 && fase == 0) {
    Serial.print(F("bestand: "));
    for (int i = 0; i < sizeof(received); i++) {
      instruction[i] = received[i];
      fase = 4;
    }
  }

  else if (strcmp(received, "list") == 0 && fase == 0) {
    VisualizeProcesTable();
  }

  else if (strcmp(received, "clearStack") == 0 && fase == 0) {
    Serial.println(F("proces"));
    for (int i = 0; i < sizeof(received); i++) {
      instruction[i] = received[i];
      fase = 4;
    }
  }

  else if (strcmp(received, "suspend") == 0 && fase == 0) {
    Serial.print(F("id: "));
    for (int i = 0; i < sizeof(received); i++) {
      instruction[i] = received[i];
      fase = 4;
    }
  }

  else if (strcmp(received, "resume") == 0 && fase == 0) {
    Serial.print("id: ");
    for (int i = 0; i < sizeof(received); i++) {
      instruction[i] = received[i];
      fase = 4;
    }
  }
  else if (strcmp(received, "kill") == 0 && fase == 0) {
    Serial.print("id: ");
    for (int i = 0; i < sizeof(received); i++) {
      instruction[i] = received[i];
      fase = 4;
    }
  }

  else if (strcmp(received, "geheugen") == 0 && fase == 0) {
    visualizeMemoryGeheugen();
  }

  else if (strcmp(received, "\0" ) != 0 && fase == 0) {
    Serial.println(F("store")); Serial.println(F("retrieve")); Serial.println(F("erase")); Serial.println(F("files")); Serial.println(F("freespace")); Serial.println(F("run")); Serial.println(F("list"));
    Serial.println(F("suspend")); Serial.println(F("resume")); Serial.println(F("kill")); Serial.println(F("memoryTable")); Serial.println(F("getVar")); Serial.println(F("storeProg")); Serial.println(F("geheugen"));
  }

  //-----------------------------------------------------
  //variable Bestand opslaan
  else if (fase == 1 and received[0] > 0) {
    for (int i = 0; i < sizeof(received); i++) {
      variable1[i] = received[i];
    }
    Serial.println(variable1);
    fase = 2;
    Serial.print(F("grootte: "));
  }

  //variable grootte opslaan
  else if (fase == 2 and received[0] > 0) {
    for (int i = 0; i < sizeof(received); i++) {
      variable2[i] = received[i];
    }
    Serial.println(variable2);
    fase = 3;
    Serial.print(F("data(max24): "));
  }


  else if (fase == 3 and received[0] > 0) {
    for (int i = 0; i < sizeof(received); i++) {
      variable3[i] = received[i];
    }
    Serial.println(variable3);
    fase = 0;
    store(variable1, variable2, variable3);
    Serial.println(F("Voer uw command in..."));
    clearAllVariables();
  }
  //---------------------------------------
  else if (fase == 5 and received[0] > 0) {
    strcpy(variable1, received);
    Serial.println(variable1);
    Serial.println(F("proces_ID: "));
    fase = 6;
  }

  else if (fase == 6 and received[0] > 0) {
    strcpy(variable2, received);
    Serial.println(variable2);
    GetUitGeheugen(variable1[0], atoi(variable2));
    fase = 0;
  }
  //---------------------------------------
  else if (fase == 4 and received[0] > 0) {
    for (int i = 0; i < sizeof(received); i++) {
      variable1[i] = received[i];
    }
    Serial.println(variable1);
    if (strcmp(instruction, "retrieve") == 0) {
      retrieve(variable1);
    }
    if (strcmp(instruction, "erase") == 0) {
      ERASE(variable1);
    }

    if (strcmp(instruction, "run") == 0) {
      RUN(variable1);
    }

    if (strcmp(instruction, "resume") == 0) {
      RESUME(atoi(variable1));
    }

    if (strcmp(instruction, "suspend") == 0) {
      SUSPEND(atoi(variable1));
    }

    if (strcmp(instruction, "kill") == 0) {
      KILL(atoi(variable1));
    }

    if (strcmp(instruction, "clearStack") == 0) {
      clearStack(atoi(variable1));
    }

    if (strcmp(instruction, "storeProg") == 0) {
      STOREPROG(variable1);
    }

    clearAllVariables();
    fase = 0;
    Serial.println(F("Voer uw command in..."));
  }
}

//Krijgt grote van het type.
void getTypeOfStack(int proces) {
  tempType = popByte(proces);
  if (tempType == INT) {
    amoutBytes = 2;
  }

  if (tempType == CHAR) {
    amoutBytes = 1;
  }

  if (tempType == FLOAT) {
    amoutBytes = 4;
  }

  if (tempType == STRING) {
    amoutBytes = popByte(proces) + 1;
  }
}
//Deze doet het geloof ik
void vanStackNaarGeheugen(int beginPositie, int amoutOfBytes, int proces) {
  for (int i = beginPositie + amoutOfBytes; i > beginPositie; i--) {
    byte temp = popByte(proces);
    geheugen[i - 1] = temp;
  }
  noOfVars++;
}

void plaatsGroote() {
  memory_table[24].naam = 'a';
  memory_table[24].adres = 255;
  memory_table[24].grootte = 0;
  memory_table[24].proces_ID = 100;
}

void checkInMemoryTabel() {
  for (int i = 0; i < 25; i++) {
    if (memory_table[i].naam == 0 and memory_table[i].proces_ID == 0) {
      int beginAdress = memory_table[i].adres;
      for (int j = i + 1; j < 25; j++) {
        if (memory_table[j].naam != 0 and memory_table[j].proces_ID != 0) {
          int eindAdress = memory_table[j].adres;
          if ((eindAdress - beginAdress) > amoutBytes) {
            Serial.print(F("Plek op: ")); Serial.print(i); Serial.print(F(" te grote van ")); Serial.print(eindAdress - beginAdress); Serial.println();
            tempBeginPositieArray = i;
            tempBeginPositie = beginAdress;
            return;
          }
        }
      }
    }
  }
}

void visualizeMemoryTabel() {
  for (int i = 0; i < 25; i++) {
    Serial.print(char(memory_table[i].naam)); Serial.print(F(" ")); Serial.print(memory_table[i].proces_ID); Serial.print(F(" ")); Serial.print(memory_table[i].grootte); Serial.print(F(" ")); Serial.print(memory_table[i].adres); Serial.println();
  }
}

void visualizeMemoryGeheugen() {
  for (int i = 0; i < 25; i++) {
    Serial.println(geheugen[i]);
  }
}

void deleteVariable(int proces_id) {
  for (int i = 0; i < 25; i++) {
    if (memory_table[i].proces_ID == proces_id) {
      noOfVars--;
      memory_table[i].naam = 0;
      memory_table[i].grootte = 0;
      memory_table[i].proces_ID = 0;
      if (memory_table[i].adres != 0 and memory_table[i - 1].naam != 0) {
        memory_table[i].adres = memory_table[i - 1].adres + memory_table[i - 1].grootte;
      }
      else {
        memory_table[i].adres = 0;
      }
    }
  }
}


void readGeheugen (byte naam, int proced_id) {
  for (int i = 0; i < 25; i++) {
    if (memory_table[i].naam == naam and memory_table[i].proces_ID == proced_id) {
      for (int a = 0 ; a < memory_table[i].grootte; a++) {
        Serial.println(geheugen[memory_table[i].adres + a]);
        pushByte(geheugen[memory_table[i].adres + a], proced_id);
      }
      if (memory_table[i].type == STRING) {
        pushByte(memory_table[i].type, proced_id);
      }
      pushByte(memory_table[i].type, proced_id);
      return;
    }
  }
}

void writeByte(byte naam, int proces_id) {
  //Kijken of er plek over is
  bool welPlek = false;
  for (int i = 0; i < 25; i++) {
    if (memory_table[i].naam == '\0') {
      welPlek = true;
      break;
    }
  }
  if (welPlek == false) {
    Serial.println(F("Er is geen plek!"));
  }
  //Dubbele naam of proces_id + wissen
  if (welPlek == true) {
    for (int i = 0; i < 25; i++) {
      if (memory_table[i].naam == naam and (memory_table[i].proces_ID == proces_id)) {
        Serial.print(F("Deze naam en proces-id bestaan op plek: ")); Serial.print(i); Serial.println();
        for (int j = i; j < 24; j++) {
          memory_table[j] = memory_table[j + 1];
        }
        noOfVars--;
        break;
      }
    }
    getTypeOfStack(proces_id);

    checkInMemoryTabel();
    //De stack vullen!
    vanStackNaarGeheugen(tempBeginPositie, amoutBytes, proces_id);

    memory_table[tempBeginPositieArray].naam = naam;
    memory_table[tempBeginPositieArray].type = tempType;
    memory_table[tempBeginPositieArray].adres = tempBeginPositie;
    memory_table[tempBeginPositieArray].grootte = amoutBytes;
    memory_table[tempBeginPositieArray].proces_ID = proces_id;
    if (memory_table[tempBeginPositieArray + 1].naam == 0) {
      memory_table[tempBeginPositieArray + 1].adres = tempBeginPositie + amoutBytes;
    }
    plaatsGroote();
  }
}

void GetUitGeheugen(char naam, int proces) {
  for (int i = 0; i < 25; i++) {
    if (memory_table[i].naam == naam and memory_table[i].proces_ID == proces) {
      if (memory_table[i].type == INT) {
        int a = geheugen[memory_table[i].adres];
        int b = geheugen[memory_table[i].adres + 1];
        Serial.println(b + (a * 256));
        return;
      }
      if (memory_table[i].type == CHAR) {
        Serial.println(char(geheugen[memory_table[i].adres]));
        return;
      }

      if (memory_table[i].type == FLOAT) {
        Serial.println(F("GEVONDEN!"));
        byte b[4];
        float *pf = (float *)b;

        for (int q = 0; q < 4; q++) {
          b[q] = int(geheugen[memory_table[i].adres + (3 - q)]);
          Serial.println(b[q]);
        }
        Serial.println(*pf);
        return;
      }
      if (memory_table[i].type == STRING) {
        Serial.println(F("string!"));
        int index = 0;
        char temp[12] = "";
        for (int q = memory_table[i].adres; q < memory_table[i].adres + memory_table[i].grootte; q++) {
          temp[index] = geheugen[q];
          index++;
        }
        Serial.println(temp);
        return;
      }
    }
  }
}
