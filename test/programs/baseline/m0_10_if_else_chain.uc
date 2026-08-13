// m0_10: if / else if / else chain
int classify(int x) {
    int r = 0;
    if (x >  100) { r =  2; }
    else if (x >   0) { r =  1; }
    else if (x ==  0) { r =  0; }
    else if (x > -100) { r = -1; }
    else              { r = -2; }
    return r;
}

int main() {
    return classify(0) + classify(50) + classify(-200);  // 255
}
