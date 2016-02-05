//
//  main.cpp
//  Practice
//
//  Created by Levi Mollison on 2/2/16.
//  Copyright © 2016 Levi. All rights reserved.
//

#include <iostream>
#include <string>
using namespace std;
#include "classes.cpp"
#include "vectors.cpp"

// makes sure the program knows this function will exist
void basics();
void runClasses();

int main () {
    // basics();
    runClasses();
    return 0;
}

void basics() {
    string name = "world";
    cout << "What's your name? ";
    cin >> name;
    cout << "Hello " << name << "!\n";
    
    int answer = 41;
    // A simple if statement
    if (answer == 42)
        cout << "Now, what was the question?\n";
    
    // A simple if-else statment
    if (answer == 42)
        cout << "Now, what was the question?\n";
    else
        cout << "The answer wasn't 42\n";
};

void runClasses(){
    // Naturally defines the variable using a default constructor
    // Simplest x;
    Simple simpleton;
    simpleton.display();
    
    Vorlon v1("Felix");
    // uses a copy constructor that's given to us
    Vorlon v2(v1);
    // uses a copy constructor as well
    Vorlon v4  = v1;
    
    v1.display();
}