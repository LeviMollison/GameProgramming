//
//  classes.cpp
//  Practice
//
//  Created by Levi Mollison on 2/2/16.
//  Copyright © 2016 Levi. All rights reserved.
//

#include <stdio.h>
#include <string>
#include <iostream>
using namespace std;



class Simplest {};

class Simple{
public:
    void display(){
        cout << "Displaying a simple object\n";
    }
};

class Vorlon{
public:
    Vorlon(const string& aName) : myName(aName) {}
    void display() const {cout << "Displaying a Vorlon named " << myName << endl;}
private:
    string myName;
};

