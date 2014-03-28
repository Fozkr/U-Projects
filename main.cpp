#include <fstream>
#include <iostream>
using namespace std;

bool cleanCode(istream* input, ostream* output);

int main(int argc, char* argv[])
{
    //First let us identify the stream we will be using
    istream* inputStream = &cin;
    ostream* outputStream = &cout;
    ifstream inputFile;
    ofstream outputFile;
    if(argc > 1)
    {
        inputFile.open(argv[1]);
        inputStream = &inputFile;
    }
    if(argc > 2)
    {
        outputFile.open(argv[2]);
        outputStream = &outputFile;
    }

    //Then let us start reading from that stream, but using a separate function
    cleanCode(inputStream, outputStream);

    if(argc > 1)
        inputFile.close();
    if(argc > 2)
        outputFile.close();

    return 0;
}

bool cleanCode(istream* input, ostream* output)
{
    //char line[256];
    string line;
    short unsigned indentationLevel = 0;
    short unsigned spaceAmount = 0;
    short spaceNeeded = 0;
    while(!input->eof()) //Read the line, then analyze each character in it, then write it
    {
        //input->getline(line, 256);
        getline(*input, line);
        char currentChar = '\0';
        spaceAmount = 0;
        for(unsigned short i=0; (currentChar = line[i])!='\0'; ++i)
        {
            switch(currentChar)
            {
                case '{':
                    ++indentationLevel;
                    ++spaceAmount; //The { symbol should not be indented
                    cout << indentationLevel << endl;
                    break;
                case '}':
                    --indentationLevel;
                    cout << indentationLevel << endl;
                    break;
                case '\t':
                    ++spaceAmount;
                    cout << "sA: " << spaceAmount << endl;
                    break;
                default:
                    break;
            }
        }
        spaceNeeded = indentationLevel - spaceAmount;
        cout << "space needed: " << spaceNeeded << endl;
        (*output) << line << endl;
        if(spaceNeeded > 0)
            line.insert(0, 3, '.');
    }
    return false;
}
