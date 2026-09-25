//***************************************************************************
//*                                                                         *
//*              WiZARD OF WOR - PC Version V 0.3.3 in C++                  *
//*                                                                         *
//*              Nov 1997 von Thomas Maag und Gerald Franz                  *
//*            und Christian Kessler-Deac und Matthias Franz                *
//*                 2025-02-26 ported to lores.h by GF                      *
//*                                                                         *
//***************************************************************************

// gcc -Os -s lowiz.cpp -o lowiz -lasound -lpthread -lm
// gcc -Os -s lowiz.cpp -o lowiz

#define LORES_H_IMPLEMENTATION
#include "lores.h"
#include "sprites.c"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define RICHTUNG int
#define LI 0
#define RE 1
#define OB 2
#define UN 3

const int FeldX = 16;
const int FeldY = 10;
const int ScreenMaxX = 76;
const int ScreenMaxY = 46;
const int MaxHero    = 2;
const int MaxMonster = 10;
const int MaxShoot   = 4;

// Klassendefinitionen ------------------------------------------------------

class ActorObj {
 public:
    char PosX;
    char PosY;
    RICHTUNG Richt;
    char Lebt;

    void Init();
    void Paint(char ch);
    void Clean();
    void Move(RICHTUNG Ri);
    void Fire();
};

class Monster1Obj : public ActorObj {
    static constexpr uint8_t numFrames = 2;
    static uint8_t* sprite[4*numFrames];
    uint8_t frame;
public:
    Monster1Obj();
    void Paint();
    void Go();
    void Move(RICHTUNG Ri);
    void Done();
};

class HeroObj : public ActorObj {
    static constexpr uint8_t numFrames = 3;
    uint8_t* sprite[4*numFrames];
public:
    int Lives;
    uint8_t frame;
    void Init(char Farbe);
    void Paint();
    void Move(RICHTUNG Ri);
    void Collision();
    void Die();
    void Fire();
    void Done();
};

class PitObj {
 public:
    char Pixel[FeldX*5][FeldY*5];

    PitObj(); // Konstruktor
    void Paint();
};


class ShootObj {
public:
    char PosX;
    char PosY;
    RICHTUNG Richt;
    char Lebt;

    void Init(int X, int Y, RICHTUNG Ri);
    void Paint();
    void Clean();
    void Move();
    char Collision(int X, int Y);
    void Go();
    void Done();
};

//--- Globale Variablen -----------------------------------------------------
PitObj Pit;
ShootObj Shoot[MaxShoot];
Monster1Obj Monster[MaxMonster];
HeroObj Hero[MaxHero];
int Spieler=MaxHero;
int Gegner=MaxMonster;
BlockBuf* bb = NULL;

static int wrapCoordinate(int value, int extent) {
  return (value % extent + extent) % extent;
}

char WandKollision(int WoX, int WoY); // forward-Deklaration

//--- ActorObj, Stammobjekt fuer Monster, Heroen, etc... --------------------

void ActorObj::Init() {
  PosX=(randi(FeldX-1))*5+1;
  PosY=(randi(FeldY-1))*5+1;
  Lebt=1;
  Richt=RE;
}

void ActorObj::Paint(char ch) {
  for(char i=0;i<4;i++)
    for(char j=0;j<4;j++)
       Pit.Pixel[PosX+i][PosY+j]=ch;
}

void ActorObj::Clean() {
  for(char j=0;j<=3;j++)
    for(char i=0;i<=3;i++)
      Pit.Pixel[PosX+i][PosY+j]=' ';
  BlockBufFill(bb,PosX*2,PosY*2,8,8,0);
}

void ActorObj::Move(RICHTUNG Ri) {
  if(!Lebt) return;
  Clean();
  Richt=Ri;
  switch(Ri) {
    case LI :
      if(!WandKollision(PosX-1,PosY)) PosX--;
      else {
        randi(2)==1 ? Move(UN) : Move(OB);
        return;
      }
      break;
    case RE :
      if(!WandKollision(PosX+1,PosY)) PosX++;
      else {
        randi(2)==1 ? Move(UN) : Move(OB);
        return;
      }
      break;
    case OB :
      if(!WandKollision(PosX,PosY-1)) PosY--;
      else {
        randi(2)==1 ? Move(LI) : Move(RE);
        return;
      }
      break;
    case UN :
      if(!WandKollision(PosX,PosY+1)) PosY++;
      else {
        randi(2)==1 ? Move(LI) : Move(RE);
      }
      break;
  }

  if(PosX<0) PosX=ScreenMaxX-1;
  if(PosY<0) PosY=ScreenMaxY-1;
  if(PosX>=ScreenMaxX) PosX=0;
  if(PosY>=ScreenMaxY) PosY=0;
}

void ActorObj::Fire() {
}

//--- Monster1 Objekt -------------------------------------------------------

uint8_t* Monster1Obj::sprite[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

Monster1Obj::Monster1Obj() : ActorObj() {
  if(sprite[0] == nullptr) {
    const char colorMap[] = { 2, '*', COLOR_RED, 'O',COLOR_LIGHT_GREEN };
    for(int fr=0; fr<numFrames; ++fr) {
      sprite[RE + fr*4] = BlockBufDecode(monster + 64*fr, 8,8, colorMap, 0);
      sprite[LI + fr*4] = BlockBufDecode(monster + 64*fr, 8,8, colorMap, FLIP_X);
      sprite[UN + fr*4] = BlockBufDecode(monster + 64*fr, 8,8, colorMap, FLIP_D|FLIP_Y);
      sprite[OB + fr*4] = BlockBufDecode(monster + 64*fr, 8,8, colorMap, FLIP_D|FLIP_X|FLIP_Y);
    }
  }
  frame = randi(numFrames);
}

void Monster1Obj::Paint() {
  if(!Lebt) return;
  ActorObj::Paint('m');
  BlockBufBlt(bb, PosX*2,PosY*2, 8,8, sprite[Richt+4*frame]);
}

void Monster1Obj::Move(RICHTUNG Ri) {
  ActorObj::Move(Ri);
  frame = (frame + 1) % numFrames;
}

void Monster1Obj::Go() {
  if(!Lebt)
    return;
  if(randi(10)==1) Richt=randi(4);
  Move(Richt);
  Paint();
}

void Monster1Obj::Done() {
  if(!Lebt)
    return;
  Gegner--;
  Clean();
  Lebt=0;
  sound(220,100);
}

//--- Hero Objekt -----------------------------------------------------------

void HeroObj::Init(char Farbe) {
  ActorObj::Init();
  frame = randi(2);
  const char colorMap[] = { 1, '*', Farbe };
  for(int fr=0; fr<numFrames; ++fr) {
    sprite[RE+4*fr] = BlockBufDecode(gunner+64*fr, 8,8, colorMap, FLIP_X);
    sprite[LI+4*fr] = BlockBufDecode(gunner+64*fr, 8,8, colorMap, 0);
    sprite[OB+4*fr] = BlockBufDecode(gunner+64*fr, 8,8, colorMap, FLIP_D|FLIP_Y);
    sprite[UN+4*fr] = BlockBufDecode(gunner+64*fr, 8,8, colorMap, FLIP_D|FLIP_X|FLIP_Y);
  }

  Lives=3;
  Paint();
}

void HeroObj::Paint() {
  if(!Lives) return;
  ActorObj::Paint('h');
  BlockBufBlt(bb, PosX*2,PosY*2, 8,8, sprite[Richt+4*frame]);
}

void HeroObj::Move(RICHTUNG Ri) {
  if(!Lives)
    return;
  ActorObj::Move(Ri);
  frame = (frame + 1) % 2;
  Paint();
}

void HeroObj::Die() {
  if(!Lives) return;
  Lives--;
  if(Lives<=0)
    Done();
  else {
    Clean();
    PosX=(randi(FeldX-1))*5+1;
    PosY=(randi(FeldY-1))*5+1;
    Paint();
  }
  sound(160,250);
}

void HeroObj::Fire() {
  if(!Lives) return;
  int i=0;
  while(i<MaxShoot && Shoot[i].Lebt) i++;
  if(i>=MaxShoot)
    return;
  sound(1000,100);
  Shoot[i].Init(PosX,PosY,Richt);
  Clean();
  frame = 2;
  Paint();
}

void HeroObj::Collision() {
  if(!Lives) return;

  for(char i=0;i<=3;i++)
    for(char j=0;j<=3;j++)
      if(Pit.Pixel[PosX+i][PosY+j]=='m') {
        Die();
        return;
      }
}

void HeroObj::Done() {
  Lives=0;
  Clean();
  Spieler--;
}


//--- Pit Objekt ------------------------------------------------------------

PitObj::PitObj() {
  int i,j,k,l;
  const char* Feld[] = { 
    "F------.-------I",
    "I-------------.I",
    "I-------------.I",
    "I              I",
    "F------IF------I",
    "I      II      I",
    "I      II      I",
    "I      II      I",
    "I      ..      I",
    "-------.-------.",
  };

  for(i=0;i<FeldX;i++)
    for(j=0;j<FeldY;j++)
      for(k=0;k<5;k++)
        for(l=0;l<5;l++)
          switch(Feld[j][i]) {
            case ' ':
              Pixel[5*i+k][5*j+l]=' ';
              break;
            case '-':
              if(l==0) Pixel[5*i+k][5*j+l]='*';
              else Pixel[5*i+k][5*j+l]=' ';
              break;
            case 'I':
              if(k==0) Pixel[5*i+k][5*j+l]='*';
              else Pixel[5*i+k][5*j+l]=' ';
              break;
            case 'F':
              if((k==0)||(l==0)) Pixel[5*i+k][5*j+l]='*';
              else Pixel[5*i+k][5*j+l]=' ';
              break;
            case '.':
              if((k==0)&&(l==0)) Pixel[5*i+k][5*j+l]='*';
              else Pixel[5*i+k][5*j+l]=' ';
              break;
          }

}

void PitObj::Paint() {
  for(int i=0;i<ScreenMaxY;i++) {
    for(int j=0;j<ScreenMaxX;j++)
      if(Pixel[j][i]=='*')
        BlockBufFill(bb, j*2,i*2, 2,2, COLOR_WHITE);
  }
}

//--- Shoot Objekt ----------------------------------------------------------

void ShootObj::Init(int X, int Y, RICHTUNG Ri) {
  Lebt=1;
  Richt=Ri;
  switch(Ri) {
    case LI:
      PosX=X-1;
      PosY=Y+1;
      break;
    case RE:
      PosX=X+4;
      PosY=Y+1;
      break;
    case OB:
      PosX=X+1;
      PosY=Y-1;
      break;
    case UN:
      PosX=X+1;
      PosY=Y+4;
      break;
  }
  PosX=wrapCoordinate(PosX,ScreenMaxX);
  PosY=wrapCoordinate(PosY,ScreenMaxY);
  if(Collision(PosX,PosY)) Lebt=0;
  if(Lebt) Paint();
}

void ShootObj::Paint() {
  if(!Lebt) return;
  BlockBufFill(bb, PosX*2,PosY*2, 2,2, COLOR_YELLOW);
}

void ShootObj::Clean() {
  if(!Lebt) return;
  BlockBufFill(bb, PosX*2,PosY*2, 2,2, COLOR_BLACK);
}

void ShootObj::Move() {
  Clean();
  switch(Richt) {
    case LI :
      if(!Collision(PosX-1,PosY)) PosX--;
      else Done();
      break;
    case RE :
      if(!Collision(PosX+1,PosY)) PosX++;
      else Done();
      break;
    case OB :
      if(!Collision(PosX,PosY-1)) PosY--;
      else Done();
      break;
    case UN :
      if(!Collision(PosX,PosY+1)) PosY++;
      else Done();
  }
  if(!Lebt) return;

  if(PosX<0) PosX=ScreenMaxX-1;
  if(PosY<0) PosY=ScreenMaxY-1;
  if(PosX>=ScreenMaxX) PosX=0;
  if(PosY>=ScreenMaxY) PosY=0;

  Paint();
}

char ShootObj::Collision(int X, int Y) {
  X=wrapCoordinate(X,ScreenMaxX);
  Y=wrapCoordinate(Y,ScreenMaxY);
  int i;
  switch(Pit.Pixel[X][Y]) //es fehlt Abfrage ob Kollision mit anderem Schuss
  {
    case '*' :
      return '*';
    case 'm' :
      for(i=0;i<MaxMonster;i++)
        if((Monster[i].PosX<=X)&&(Monster[i].PosX+4>=X)&&
          (Monster[i].PosY<=Y)&&(Monster[i].PosY+4>=Y))
          Monster[i].Done();
      return 'm';
    case 'h' :
      for(i=0;i<MaxHero;i++)
        if((Hero[i].PosX<=X)&&(Hero[i].PosX+4>=X)
          &&(Hero[i].PosY<=Y)&&(Hero[i].PosY+4>=Y))
          Hero[i].Die();
      return 'h';
    default :
      return 0;
  }
}

void ShootObj::Go() {
  char i=1;
  while((i<5)&&Lebt)
  {
    Move();
    i++;
  }
}

void ShootObj::Done() {
  if(!Lebt) return;
  Lebt=0;
  Clean();
}

//--- Hauptprogramm --------------------------------------------------------

char WandKollision(int WoX, int WoY) {
  WoX=wrapCoordinate(WoX,ScreenMaxX);
  WoY=wrapCoordinate(WoY,ScreenMaxY);
  int i,j;
  for(i=0;i<=3;i++)
    for(j=0;j<=3;j++)
      if(Pit.Pixel[WoX+i][WoY+j]=='*') return 1;
  return 0;
}

int main() {
  char ch=' ';
  unsigned long Runde=0;
  int i;

  initscr();
  atexit(shutdownscr);
  echooff();
  cursoroff();
  clrscr();

  bb = BlockBufCreate(ScreenMaxX*2, ScreenMaxY*2);
  Pit.Paint();
  for(i=0;i<MaxHero;i++) Hero[i].Init(i+4);
  for(i=0;i<MaxShoot;i++) Shoot[i].Lebt=0;
  for(i=0;i<MaxMonster;i++) Monster[i].Init();

  do {
    if(kbhit()) {
      ch=getkey();
      switch(ch) {
      case KEY_LEFT :
        Hero[0].Move(LI);
        break;
      case KEY_RIGHT :
        Hero[0].Move(RE);
        break;
      case KEY_UP :
        Hero[0].Move(OB);
        break;
      case KEY_DOWN :
        Hero[0].Move(UN);
        break;
      case KEY_ENTER :
      case ' ':
        Hero[0].Fire();
        break;

      case 'w' :
        Hero[1].Move(OB);
        break;
      case 's' :
        Hero[1].Move(UN);
        break;
      case 'a' :
        Hero[1].Move(LI);
        break;
      case 'd' :
        Hero[1].Move(RE);
        break;
      case KEY_TAB :
        Hero[1].Fire();
        break;
      }
    }

    for(i=0;i<MaxHero;i++) if(Hero[i].Lives) Hero[i].Paint();
    for(i=0;i<MaxMonster;i++) if(Monster[i].Lebt) Monster[i].Go();
    for(i=0;i<MaxShoot;i++) if(Shoot[i].Lebt) Shoot[i].Go();
    for(i=0;i<MaxHero;i++) if(Hero[i].Lives) Hero[i].Collision();
    BlockBufOut(bb,1,1);

    gotoxy(1,47);
    textcolor(COLOR_LIGHT_YELLOW);
    printf("Spieler 1 Leben : %d Spieler 2 Leben : %d Monster : %d Runde : %lu",
      Hero[0].Lives, Hero[1].Lives, Gegner, ++Runde);

    delay(50);
  } while((ch!=KEY_ESCAPE) && Spieler && Gegner);

  free(bb);
  shutdownscr();
  return 0;
}
