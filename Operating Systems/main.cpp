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
bool cleanCode(istream* input, ostream* output, const short unsigned spaceAmount, short unsigned* wordsCount);
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

    //Second, create an array with all the reserved words to be identified and another one to count those
    string reservedWords[] = {"bool", "break", "case", "catch", "char", "class", "const", "continue", "default",
    "delete", "do", "double", "else", "enum", "explicit", "extern", "false", "float", "for", "friend", "if",
    "inline", "int", "long", "namespace", "new", "operator", "private", "protected", "public", "register",
    "return", "short", "signed", "sizeof", "static", "struct", "switch", "template", "true", "try", "typedef",
    "typeid", "typename", "unsigned", "using", "virtual", "void", "volatile", "while"};
    short unsigned wordsCount[50] = {0};
    //TODO: count the words in the cleanCode function!!!

    //Then let us identify the stream we will be using
    istream* inputStream = &cin;
    ostream* outputStream = &cout;
    if(inputFile.is_open())
        inputStream = &inputFile;
    if(outputFile.is_open())
        outputStream = &outputFile;

    //Then let us start reading from that stream, but using a separate function
    cleanCode(inputStream, outputStream, spaceAmount, wordsCount);

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

//Analyzes the input, line by line, char by char, looking for unnecesary spaces or the lack of them
bool cleanCode(istream* input, ostream* output, const short unsigned spaceAmount, short unsigned* wordsCount)
{
    string line;
    short unsigned indentationLevel = 0;
    short unsigned currentPos = 0;
    char currentChar = '\0';
    while(!input->eof()) //Read the line, analyze each character in it, then write it, until the eof is reached
    {
        getline(*input, line);
        cout << line << endl; //*****
        currentPos = 0;

        //First, check indentation, except in empty lines (less storage space)
        if(line.size() > 0)
            checkIndentation(line, currentChar, currentPos, indentationLevel, spaceAmount);

        //Second, check the rest of the line
        for(; (currentChar = line[currentPos])!='\0'; ++currentPos)
        {
            //cout << currentChar << endl; //*****
            switch(currentChar)
            {
                case '/':
                    cout << "'" << line[currentPos+1] << "'" << endl;
                    if(line[currentPos+1] == '/') //If it starts with "//" it is a comment, leave it alone
                        //currentPos = line.size(); //does not work if spaces were added before, apparently
                    {
                        while(line[currentPos] != '\0')
                            ++currentPos;
                    }
                    break;
                case '"':
                    ++currentPos;
                    while(line[currentPos] != '"' || line[currentPos-1] == '\\') //just in case there is a \" in the string
                        ++currentPos; //fast forward until the end of the string
                    ++currentPos;
                    break;
                case '{':
                    if(currentPos != (indentationLevel*spaceAmount)) //if it is not starting the line, it should be
                    {
                        line.insert(currentPos++, 1, '\n');
                        checkIndentation(line, currentChar, currentPos, indentationLevel, spaceAmount);
                    }
                    ++indentationLevel;
                    if(line[currentPos+1] != '\0') //if it is not ending the line, it should be
                    {
                        ++currentPos;
                        line.insert(currentPos++, 1, '\n');
                        checkIndentation(line, currentChar, currentPos, indentationLevel, spaceAmount);
                    }
                    cout << indentationLevel << endl; //*****
                    break;
                case '}':
                    if(currentPos != (indentationLevel*spaceAmount)) //if it is not starting the line, it should be
                    {
                        cout << line << endl; //******
                        cout << currentPos << "'" << line[currentPos-3] << line[currentPos-2] << line[currentPos-1] << line[currentPos] << "'" << endl; //*****
                        line.insert(currentPos++, 1, '\n');
                        checkIndentation(line, currentChar, currentPos, indentationLevel, spaceAmount);
                    }
                    if(line[currentPos+1] != '\0') //if it is not ending the line, it should be
                    {
                        ++currentPos;
                        line.insert(currentPos++, 1, '\n');
                        checkIndentation(line, currentChar, currentPos, indentationLevel, spaceAmount);
                    }
                    break;
                case ' ':
                    while(line[currentPos+1] == ' ') //Only leave one space, no more than one
                    {
                        cout << "SPACE" << endl;
                        line.erase(currentPos+1, 1);
                    }
                    break;
                case ';':
                    if(line[currentPos+1]!='\0' && line[currentPos+1]!='\n' && line[currentPos+1]!='}') //if it is not ending the line, it should be
                    {
                        ++currentPos;
                        line.insert(currentPos++, 1, '\n');
                        checkIndentation(line, currentChar, currentPos, indentationLevel, spaceAmount);
                        --currentPos;
                    }
                    break;
                case '=':
                    if(line[currentPos-1] != ' ')
                        line.insert(currentPos++, 1, ' ');
                    if(line[currentPos+1] != ' ')
                        line.insert(++currentPos, 1, ' ');
                    break;
                    //Corrupts the #include lines at the beginning of files
//                case '<':
//                case '>':
//                    if(line[currentPos-1] != ' ' && line[currentPos-1] != '<' && line[currentPos-1] != '>')
//                        line.insert(currentPos++, 1, ' ');
//                    if(line[currentPos+1] != ' ' && line[currentPos+1] != '<' && line[currentPos+1] != '>')
//                        line.insert(++currentPos, 1, ' ');
//                    break;
                default:
                    break;
            }
        }
        (*output) << line << endl;
    }
    return true;
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
        line.erase(currentPos+spacesNeeded, spacesNeeded*-1);
    currentPos += spacesNeeded; //if spaces were inserted or deleted, move the currentPos pointer accordingly
}
