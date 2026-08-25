#include <stdio.h>
#include <string.h>
#include "jit/strbuf.h"
#include "jit/usage.h"

int main() {
  warning("This is a test warning! My favourite number is %d", 42);
}