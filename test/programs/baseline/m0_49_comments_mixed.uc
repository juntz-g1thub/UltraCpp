// m0_49: Mixed line and block comments
// Single-line comment at the top.

/* Block comment that
   spans multiple lines. */

int main() {
    // line comment inside a function
    int x = 1;                 /* trailing block comment */
    /* multi-line
       block again */          int y = 2;
    //                  ^ works because comments are tokenized away
    return x + y;              // 3
}
