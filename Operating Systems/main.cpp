/* This program works as a "code cleaner", it reads source code (both from standard input or files)
 * and looks for unnecessary characters to be eliminated or the lack of necessary characters.
 * This needs are predefined according to a specific style explained in the documentation.
 * by Oscar Esquivel Oviedo, B22410, UCR
 * v2.0, 2/4/2014
 */

#include <fstream>
#include <iostream>
using namespace std;

bool checkParameters(const int argc, char* argv[], ifstream& inputFile, ofstream& outputFile, short unsigned& spaceAmount);
bool cleanCode(istream* input, ostream* output, const short unsigned spaceAmount);
void checkIndentation(string& line, char& currentChar, short unsigned& currentPos, short unsigned& indentationLevel, const short unsigned& spaceAmount);

int main(int argc, char* argv[])
{
    //Initial necessary variables
    short unsigned spaceAmount = 4; //A little variable to handle the indentation space, default 4
    ifstream inputFile; //and file streams just in case we will have to handle files
    ofstream outputFile; //they will be opened in "checkParameters" if so

    //First check the parameters, this program does not really validate the parameters
    bool recognized = checkParameters(argc, argv, inputFile, outputFile, spaceAmount);
    if(!recognized)
    {
        if(inputFile.is_open()) inputFile.close();
        if(outputFile.is_open()) outputFile.close();
        return 0;
    }

    cout << inputFile.is_open() << ' ' << outputFile.is_open() << ' ' << spaceAmount << endl; //*****

    //Then let us identify the stream we will be using
    istream* inputStream = &cin;
    ostream* outputStream = &cout;
    if(inputFile.is_open())
        inputStream = &inputFile;
    if(outputFile.is_open())
        outputStream = &outputFile;

    //Then let us start reading from that stream, but using a separate function
    cleanCode(inputStream, outputStream, spaceAmount);

    if(inputFile.is_open())
        inputFile.close();
    if(outputFile.is_open())
        outputFile.close();

    return 0;
}

//Tries to quickly recognize the parameters sent to the program, does not check if files exist
bool checkParameters(const int argc, char* argv[], ifstream& inputFile, ofstream& outputFile, short unsigned& spaceAmount)
{
    bool recognized = true;
    for(short unsigned i=1; i<argc; ++i)
    {
        cout << argv[i] << endl; //*****
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
                case 'e':
                    spaceAmount = argv[i][2] - 48; //Should be immediatly followed by the amount of spaces, only values from 0 to 9 accepted
                    break;
                case 'o':
                    outputFile.open(argv[++i]); //Should be followed by the file name, so that next parameter should be ignored in the next loop
                    break;
                default:
                    recognized = false; //Any other -x option is not recognized
            }
        }
        else
            inputFile.open(argv[i]); //If it does not start with a '-', it is assumed it is the input file name
        if(!recognized || i>4) //There should not be more than 5 arguments, and they all should be recognized, report if otherwise
        {
            cout << "Invalid parameter: '" << argv[i] << "'\nTerminating program";
            return false;
        }
    }
    return true;
}

bool cleanCode(istream* input, ostream* output, const short unsigned spaceAmount)
{
    string line;
    short unsigned indentationLevel = 0;
    short unsigned currentPos = 0;
    char currentChar = '\0';
    while(!input->eof()) //Read the line, analyze each character in it, then write it, until the eof is reached
    {
        getline(*input, line); cout << line << endl; //*****
        //currentChar = '\0';
        currentPos = 0;

        //First, check indentation
        checkIndentation(line, currentChar, currentPos, indentationLevel, spaceAmount);
//        for(; (currentChar = line[currentPos])==' '; ++currentPos)
//            ++spaces;
//        if(line[currentPos] == '}')
//            --indentationLevel;
//        spacesNeeded = indentationLevel*spaceAmount - spaces;
//        cout << "spaces needed: " << spacesNeeded << endl; //*****
//        if(spacesNeeded > 0)
//            line.insert(0, spacesNeeded, ' ');
//        else if(spacesNeeded < 0)
//            line.erase(0, spacesNeeded*-1);
//        currentPos += spacesNeeded; //if spaces were inserted or deleted, move the currentPos pointer accordingly

        //Second, check the rest
        for(; (currentChar = line[currentPos])!='\0'; ++currentPos)
        {
            //cout << currentChar << endl; //*****
            switch(currentChar)
            {
                case '/':
                    if(line[currentPos+1] == '/') //If it starts with "//" it is a comment, leave it alone
                        currentPos = line.size();
                    break;
                case '"':
                    ++currentPos;
                    while(line[currentPos] != '"')
                        ++currentPos; //fast forward until the end of the string
                    ++currentPos;
                    break;
                case '{': //TODO: tell that if it isn't the first char in the line, then put a \n before it
                    ++indentationLevel;
                    if(line[++currentPos] != '\0')
                    {
                        line.insert(currentPos++, 1, '\n');
                        checkIndentation(line, currentChar, currentPos, indentationLevel, spaceAmount);
                    }
                    cout << indentationLevel << endl; //*****
                    break;
                case '}': //TODO: insert both \n before and after where appropiate
                    --indentationLevel;
                    cout << indentationLevel << endl; //*****
                    break;
                case ' ':
                    while(line[currentPos+1] == ' ') //Only leave one space, no more than one
                    {
                        cout << "SPACE" << endl; //*****
                        line.erase(currentPos+1, 1);
                    }
                    break;
                default:
                    break;
            }
        }
        (*output) << line << endl;
    }
    return false;
}

void checkIndentation(string& line, char& currentChar, short unsigned& currentPos, short unsigned& indentationLevel, const short unsigned& spaceAmount)
{
    short unsigned spaces = 0;
    short spacesNeeded = 0;
    for(; (currentChar = line[currentPos])==' '; ++currentPos)
            ++spaces;
    if(line[currentPos] == '}')
        --indentationLevel;
    spacesNeeded = indentationLevel*spaceAmount - spaces;
    cout << "spaces needed: " << spacesNeeded << endl; //*****
    if(spacesNeeded > 0)
        line.insert(currentPos, spacesNeeded, ' ');
    else if(spacesNeeded < 0)
        line.erase(currentPos, spacesNeeded*-1);
    currentPos += spacesNeeded; //if spaces were inserted or deleted, move the currentPos pointer accordingly
    if(line[currentPos] == '}')
        ++currentPos; //ignore the '}' char after indentation
}
