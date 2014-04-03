#include <fstream>
#include <iostream>
using namespace std;

bool checkParameter(char* parameter, bool& inputFile, bool& outputFile, short unsigned& spaceAmount);
bool cleanCode(istream* input, ostream* output);

int main(int argc, char* argv[])
{
    //Initial boolean variables that indicate wether or not we'll be handling files
    bool inputFileSpecified = false;
    bool outputFileSpecified = false;
    short unsigned spaceAmount = 4; //plus a little variable to handle the indentation space, default 4

    //First check the parameters
    for(short unsigned i=1; i<argc; ++i)
    {
        //if(!(checkParameter(argv[i], inputFile, outputFile, spaceAmount)));
        bool recognized = checkParameter(argv[i], inputFileSpecified, outputFileSpecified, spaceAmount); //returns if the parameter was recognized or not
        if(!recognized)
        {
            cout << "Invalid parameter: '" << argv[i] << "'\nTerminating program";
            return 0;
        }
        if(outputFileSpecified) ++i; //the "-o" should be followed by the file name, so it should ignore it and assume it is a file name
    }

    cout << inputFileSpecified << ' ' << outputFileSpecified << ' ' << spaceAmount << endl; //*****

    //Then let us identify the stream we will be using
    istream* inputStream = &cin;
    ostream* outputStream = &cout;
    ifstream inputFile;
    ofstream outputFile;
    if(inputFileSpecified)
    {
        inputFile.open(argv[1]);
        inputStream = &inputFile;
    }
    if(outputFileSpecified)
    {
        outputFile.open(argv[2]);
        outputStream = &outputFile;
    }

    //Then let us start reading from that stream, but using a separate function
    //cleanCode(inputStream, outputStream);

    if(inputFileSpecified)
        inputFile.close();
    if(outputFileSpecified)
        outputFile.close();

    return 0;
}

//Tries to quickly recognize the parameters sent to the program, does not check if files exist
bool checkParameter(char* parameter, bool& inputFile, bool& outputFile, short unsigned& spaceAmount)
{
    cout << parameter << endl; //*****
    if(parameter[0] == '-')
    {
        switch(parameter[1])
        {
            case 'e':
                spaceAmount = parameter[2] - 48; //Should be followed by the amount of spaces, only values from 0 to 9 accepted
                break;
            case 'o':
                outputFile = true; //does not check if it exists, that will be checked in the main
                break;
            default:
                return false;
        }
    }
    else
        inputFile = true; //does not check if it exists, that will be checked in the main

    return true;
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
