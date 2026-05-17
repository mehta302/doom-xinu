#include <xinu.h>
#include <stdio.h>

process myoutput(did32 dev) {
  fprintf(dev, "Bro");
  fprintf(dev, "\e[1;1H\e[2J");
}
