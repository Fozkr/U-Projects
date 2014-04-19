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
void reportResults(ostream* output, const string* reservedWords, const short unsigned* wordsCount);

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

    //Finally, report results of the word count
    reportResults(outputStream, reservedWords, wordsCount);

    //Before exiting, close the file streams if they were opened
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
    bool inFor = false; //Used to know when not to add '\n's after ';'s because of being inside a for statement
    //bool inSwitch = false;
    while(!input->eof()) //Read the line, analyze each character in it, then write it, until the eof is reached
    {
        getline(*input, line);
        cout << line << endl; //*****
        currentPos = 0;
        inFor = false;

        //First, check indentation, except in empty lines (less storage space)
        if(line.size() > 0)
            checkIndentation(line, currentChar, currentPos, indentationLevel, spaceAmount);

        //Second, check the rest of the line
        for(; (currentChar = line[currentPos])!='\0'; ++currentPos)
        {
            cout << currentChar << endl; //*****
            switch(currentChar)
            {
                //Special characters that require some specific formatting
                case '/':
                    cout << "'" << line[currentPos+1] << "'" << endl;
                    if(line[currentPos+1] == '/') //If it starts with "//" it is a comment, leave it alone
                        currentPos = line.size() - 1; //does not work if spaces were added before, apparently
//                    {
//                        while(line[currentPos] != '\0')
//                            ++currentPos;
//                    }
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
                        if(line[currentPos] != '}') --currentPos;
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
                        if(line[currentPos] != '}') --currentPos;
                    }
                    cout << indentationLevel << endl; //*****
                    break;
                case ' ':
                    while(line[currentPos+1] == ' ') //Only leave one space, no more than one
                    {
                        cout << "Found unnecessary space dude" << endl;
                        cout << "'" << line[currentPos-2] << line[currentPos-1] << line[currentPos] << line[currentPos+1] << line[currentPos+2] << "'" << endl;
                        line.erase(currentPos+1, 1);
                        cout << "SPACE DELETED" << endl; //*****
                    }
                    if(line[currentPos+1] == '(' || line[currentPos+1] == ';' || line[currentPos+1] == '\n' || line[currentPos+1] == '\0')
                        line.erase(currentPos, 1);
                    break;
                case ';':
                case ':':
                    if(!inFor && line[currentPos+1]!='\0' && line[currentPos+1]!='\n' && line[currentPos+1]!='}') //if it is not ending the line, it should be
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
                case ')':
                    inFor = false;
                    break;
                    //Corrupts the #include lines at the beginning of files
//                case '<':
//                case '>':
//                    if(line[currentPos-1] != ' ' && line[currentPos-1] != '<' && line[currentPos-1] != '>')
//                        line.insert(currentPos++, 1, ' ');
//                    if(line[currentPos+1] != ' ' && line[currentPos+1] != '<' && line[currentPos+1] != '>')
//                        line.insert(++currentPos, 1, ' ');
//                    break;
                //Normal characters that may be the beginning of reserved words
                case 'b':
                    if(line[currentPos+1] == 'o' &&
                        line[currentPos+2] == 'o' &&
                        line[currentPos+3] == 'l' &&
                        line[currentPos+4] == ' ')
                        {
                                ++wordsCount[0];
                                currentPos += 3;
                        }
                    else if(line[currentPos+1] == 'r' &&
                            line[currentPos+2] == 'e' &&
                            line[currentPos+3] == 'a' &&
                            line[currentPos+4] == 'k' &&
                            (line[currentPos+5] == ';' || line[currentPos+5] == ' '))
                            {
                                ++wordsCount[1];
                                currentPos += 4;
                            }
                    break;
                case 'c':
                    if(line[currentPos+1] == 'a' &&
                        line[currentPos+2] == 's' &&
                        line[currentPos+3] == 'e' &&
                        line[currentPos+4] == ' ')
                        {
                                ++wordsCount[2];
                                currentPos += 3;
                        }
                    else if(line[currentPos+1] == 'a' &&
                            line[currentPos+2] == 't' &&
                            line[currentPos+3] == 'c' &&
                            line[currentPos+4] == 'h' &&
                            (line[currentPos+5] == '(' || line[currentPos+5] == ' '))
                            {
                                ++wordsCount[3];
                                currentPos += 4;
                            }
                    else if(line[currentPos+1] == 'h' &&
                            line[currentPos+2] == 'a' &&
                            line[currentPos+3] == 'r' &&
                            line[currentPos+4] == ' ')
                            {
                                ++wordsCount[4];
                                currentPos += 3;
                            }
                    else if(line[currentPos+1] == 'l' &&
                            line[currentPos+2] == 'a' &&
                            line[currentPos+3] == 's' &&
                            line[currentPos+4] == 's' &&
                            line[currentPos+5] == ' ')
                            {
                                ++wordsCount[5];
                                currentPos += 4;
                            }
                    else if(line[currentPos+1] == 'o' &&
                            line[currentPos+2] == 'n' &&
                            line[currentPos+3] == 's' &&
                            line[currentPos+4] == 't' &&
                            line[currentPos+5] == ' ')
                            {
                                ++wordsCount[6];
                                currentPos += 4;
                            }
                    else if(line[currentPos+1] == 'o' &&
                            line[currentPos+2] == 'n' &&
                            line[currentPos+3] == 't' &&
                            line[currentPos+4] == 'i' &&
                            line[currentPos+5] == 'n' &&
                            line[currentPos+6] == 'u' &&
                            line[currentPos+7] == 'e' &&
                            (line[currentPos+8] == ';' || line[currentPos+8] == ' '))
                            {
                                ++wordsCount[7];
                                currentPos += 7;
                            }
                    break;
                case 'd':
                    if(line[currentPos+1] == 'e' &&
                        line[currentPos+2] == 'f' &&
                        line[currentPos+3] == 'a' &&
                        line[currentPos+4] == 'u' &&
                        line[currentPos+5] == 'l' &&
                        line[currentPos+6] == 't' &&
                        line[currentPos+7] == ':')
                        {
                                ++wordsCount[8];
                                currentPos += 6;
                        }
                    else if(line[currentPos+1] == 'e' &&
                            line[currentPos+2] == 'l' &&
                            line[currentPos+3] == 'e' &&
                            line[currentPos+4] == 't' &&
                            line[currentPos+5] == 'e' &&
                            line[currentPos+6] == ' ')
                            {
                                ++wordsCount[9];
                                currentPos += 5;
                            }
                    else if(line[currentPos+1] == 'o' &&
                            (line[currentPos+2] == ' ' || line[currentPos+2] == '{' || line[currentPos+2] == '\n' || line[currentPos+2] == '\0'))
                            {
                                ++wordsCount[10];
                                currentPos += 1;
                            }
                    else if(line[currentPos+1] == 'o' &&
                            line[currentPos+2] == 'u' &&
                            line[currentPos+3] == 'b' &&
                            line[currentPos+4] == 'l' &&
                            line[currentPos+5] == 'e' &&
                            line[currentPos+6] == ' ')
                            {
                                ++wordsCount[11];
                                currentPos += 5;
                            }
                    break;
                case 'e':
                    if(line[currentPos+1] == 'l' &&
                        line[currentPos+2] == 's' &&
                        line[currentPos+3] == 'e' &&
                        (line[currentPos+4] == ' ' || line[currentPos+4] == '{' || line[currentPos+4] == '\n' || line[currentPos+4] == '\0'))
                        {
                                ++wordsCount[12];
                                currentPos += 3;
                        }
                    else if(line[currentPos+1] == 'n' &&
                            line[currentPos+2] == 'u' &&
                            line[currentPos+3] == 'm' &&
                            line[currentPos+4] == ' ')
                            {
                                ++wordsCount[13];
                                currentPos += 3;
                            }
                    else if(line[currentPos+1] == 'x' &&
                            line[currentPos+2] == 'p' &&
                            line[currentPos+3] == 'l' &&
                            line[currentPos+4] == 'i' &&
                            line[currentPos+5] == 'c' &&
                            line[currentPos+6] == 'i' &&
                            line[currentPos+7] == 't' &&
                            line[currentPos+8] == ' ')
                            {
                                ++wordsCount[14];
                                currentPos += 7;
                            }
                    else if(line[currentPos+1] == 'x' &&
                            line[currentPos+2] == 't' &&
                            line[currentPos+3] == 'e' &&
                            line[currentPos+4] == 'r' &&
                            line[currentPos+5] == 'n' &&
                            line[currentPos+6] == ' ')
                            {
                                ++wordsCount[15];
                                currentPos += 5;
                            }
                    break;
                case 'f':
                    if(line[currentPos+1] == 'a' &&
                        line[currentPos+2] == 'l' &&
                        line[currentPos+3] == 's' &&
                        line[currentPos+4] == 'e' &&
                        (line[currentPos+5] == ' ' || line[currentPos+5] == ';' || line[currentPos+5] == ')'))
                        {
                                ++wordsCount[16];
                                currentPos += 4;
                        }
                    else if(line[currentPos+1] == 'l' &&
                            line[currentPos+2] == 'o' &&
                            line[currentPos+3] == 'a' &&
                            line[currentPos+4] == 't' &&
                            line[currentPos+5] == ' ')
                            {
                                ++wordsCount[17];
                                currentPos += 4;
                            }
                    else if(line[currentPos+1] == 'o' &&
                            line[currentPos+2] == 'r' &&
                            (line[currentPos+3] == ' ' || line[currentPos+3] == '('))
                            {
                                ++wordsCount[18];
                                currentPos += 2;
                                inFor = true;
                            }
                    else if(line[currentPos+1] == 'r' &&
                            line[currentPos+2] == 'i' &&
                            line[currentPos+3] == 'e' &&
                            line[currentPos+4] == 'n' &&
                            line[currentPos+5] == 'd' &&
                            line[currentPos+6] == ' ')
                            {
                                ++wordsCount[19];
                                currentPos += 5;
                            }
                    break;
                case 'i':
                    if(line[currentPos+1] == 'f' &&
                        (line[currentPos+2] == ' ' || line[currentPos+2] == '('))
                        {
                            ++wordsCount[20];
                            currentPos += 1;
                        }
                    else if(line[currentPos+1] == 'n' &&
                            line[currentPos+2] == 'l' &&
                            line[currentPos+3] == 'i' &&
                            line[currentPos+4] == 'n' &&
                            line[currentPos+5] == 'e' &&
                            line[currentPos+6] == ' ')
                            {
                                ++wordsCount[21];
                                currentPos += 5;
                            }
                    else if(line[currentPos+1] == 'n' &&
                            line[currentPos+2] == 't' &&
                            line[currentPos+3] == ' ')
                            {
                                ++wordsCount[22];
                                currentPos += 2;
                            }
                    break;
                case 'l':
                    if(line[currentPos+1] == 'o' &&
                        line[currentPos+2] == 'n' &&
                        line[currentPos+3] == 'g' &&
                        line[currentPos+4] == ' ')
                        {
                            ++wordsCount[23];
                            currentPos += 3;
                        }
                    break;
                case 'n':
                    if(line[currentPos+1] == 'a' &&
                        line[currentPos+2] == 'm' &&
                        line[currentPos+3] == 'e' &&
                        line[currentPos+4] == 's' &&
                        line[currentPos+5] == 'p' &&
                        line[currentPos+6] == 'a' &&
                        line[currentPos+7] == 'c' &&
                        line[currentPos+8] == 'e' &&
                        line[currentPos+9] == ' ')
                        {
                            ++wordsCount[24];
                            currentPos += 8;
                        }
                    else if(line[currentPos+1] == 'e' &&
                            line[currentPos+2] == 'w' &&
                            line[currentPos+3] == ' ')
                            {
                                ++wordsCount[25];
                                currentPos += 2;
                            }
                    break;
                case 'o':
                    if(line[currentPos+1] == 'p' &&
                        line[currentPos+2] == 'e' &&
                        line[currentPos+3] == 'r' &&
                        line[currentPos+4] == 'a' &&
                        line[currentPos+5] == 't' &&
                        line[currentPos+6] == 'o' &&
                        line[currentPos+7] == 'r')
                        {
                            ++wordsCount[26];
                            currentPos += 6;
                        }
                    break;
                case 'p':
                    if(line[currentPos+1] == 'r' &&
                        line[currentPos+2] == 'i' &&
                        line[currentPos+3] == 'v'  &&
                        line[currentPos+4] == 'a' &&
                        line[currentPos+5] == 't' &&
                        line[currentPos+6] == 'e' &&
                        (line[currentPos+7] == ':' || line[currentPos+7] == ' '))
                        {
                            ++wordsCount[27];
                            currentPos += 6;
                        }
                    else if(line[currentPos+1] == 'r' &&
                            line[currentPos+2] == 'o' &&
                            line[currentPos+3] == 't' &&
                            line[currentPos+4] == 'e' &&
                            line[currentPos+5] == 'c' &&
                            line[currentPos+6] == 't' &&
                            line[currentPos+7] == 'e' &&
                            line[currentPos+8] == 'd' &&
                            (line[currentPos+9] == ':' || line[currentPos+9] == ' '))
                            {
                                ++wordsCount[28];
                                currentPos += 8;
                            }
                    else if(line[currentPos+1] == 'u' &&
                            line[currentPos+2] == 'b' &&
                            line[currentPos+3] == 'l' &&
                            line[currentPos+4] == 'i' &&
                            line[currentPos+5] == 'c' &&
                            (line[currentPos+6] == ':' || line[currentPos+6] == ' '))
                            {
                                ++wordsCount[29];
                                currentPos += 5;
                            }
                    break;
                case 'r':
                    if(line[currentPos+1] == 'e' &&
                        line[currentPos+2] == 'g' &&
                        line[currentPos+3] == 'i'  &&
                        line[currentPos+4] == 's' &&
                        line[currentPos+5] == 't' &&
                        line[currentPos+6] == 'e' &&
                        line[currentPos+8] == 'r' &&
                        line[currentPos+9] == ' ')
                        {
                            ++wordsCount[30];
                            currentPos += 8;
                        }
                    else if(line[currentPos+1] == 'e' &&
                            line[currentPos+2] == 't' &&
                            line[currentPos+3] == 'u' &&
                            line[currentPos+4] == 'r' &&
                            line[currentPos+5] == 'n' &&
                            (line[currentPos+6] == ';' || line[currentPos+6] == ' '))
                            {
                                ++wordsCount[31];
                                currentPos += 5;
                            }
                    break;
                case 's':
                    if(line[currentPos+1] == 'h' &&
                        line[currentPos+2] == 'o' &&
                        line[currentPos+3] == 'r' &&
                        line[currentPos+4] == 't' &&
                        line[currentPos+5] == ' ')
                        {
                            ++wordsCount[32];
                            currentPos += 4;
                        }
                    else if(line[currentPos+1] == 'i' &&
                            line[currentPos+2] == 'g' &&
                            line[currentPos+3] == 'n' &&
                            line[currentPos+4] == 'e' &&
                            line[currentPos+5] == 'd' &&
                            line[currentPos+6] == ' ')
                            {
                                ++wordsCount[33];
                                currentPos += 5;
                            }
                    else if(line[currentPos+1] == 'i' &&
                            line[currentPos+2] == 'z' &&
                            line[currentPos+3] == 'e' &&
                            line[currentPos+4] == 'o' &&
                            line[currentPos+5] == 'f' &&
                            line[currentPos+6] == ' ')
                            {
                                ++wordsCount[34];
                                currentPos += 5;
                            }
                    else if(line[currentPos+1] == 't' &&
                            line[currentPos+2] == 'a' &&
                            line[currentPos+3] == 't' &&
                            line[currentPos+4] == 'i' &&
                            line[currentPos+5] == 'c' &&
                            line[currentPos+6] == ' ')
                            {
                                ++wordsCount[35];
                                currentPos += 5;
                            }
                    else if(line[currentPos+1] == 't' &&
                            line[currentPos+2] == 'r' &&
                            line[currentPos+3] == 'u' &&
                            line[currentPos+4] == 'c' &&
                            line[currentPos+5] == 't' &&
                            line[currentPos+6] == ' ')
                            {
                                ++wordsCount[36];
                                currentPos += 5;
                            }
                    else if(line[currentPos+1] == 'w' &&
                            line[currentPos+2] == 'i' &&
                            line[currentPos+3] == 't' &&
                            line[currentPos+4] == 'c' &&
                            line[currentPos+5] == 'h' &&
                            (line[currentPos+6] == '(' || line[currentPos+6] == ' '))
                            {
                                ++wordsCount[37];
                                currentPos += 5;
                            }
                    break;
                case 't':
                    if(line[currentPos+1] == 'e' &&
                        line[currentPos+2] == 'm' &&
                        line[currentPos+3] == 'p' &&
                        line[currentPos+4] == 'l' &&
                        line[currentPos+5] == 'a' &&
                        line[currentPos+6] == 't' &&
                        line[currentPos+7] == 'e' &&
                        (line[currentPos+8] == ' ' || line[currentPos+8] == ')'))
                        {
                            ++wordsCount[38];
                            currentPos += 7;
                        }
                    else if(line[currentPos+1] == 'r' &&
                            line[currentPos+2] == 'u' &&
                            line[currentPos+3] == 'e' &&
                            (line[currentPos+4] == ' ' || line[currentPos+4] == ';' || line[currentPos+4] == ')'))
                            {
                                ++wordsCount[39];
                                currentPos += 3;
                            }
                    else if(line[currentPos+1] == 'r' &&
                            line[currentPos+2] == 'y' &&
                            (line[currentPos+3] == '\0' || line[currentPos+3] == '\n' || line[currentPos+3] == '{'))
                            {
                                ++wordsCount[40];
                                currentPos += 2;
                            }
                    else if(line[currentPos+1] == 'y' &&
                            line[currentPos+2] == 'p' &&
                            line[currentPos+3] == 'e' &&
                            line[currentPos+4] == 'd' &&
                            line[currentPos+5] == 'e' &&
                            line[currentPos+6] == 'f' &&
                            line[currentPos+7] == ' ')
                            {
                                ++wordsCount[41];
                                currentPos += 6;
                            }
                    else if(line[currentPos+1] == 'y' &&
                            line[currentPos+2] == 'p' &&
                            line[currentPos+3] == 'e' &&
                            line[currentPos+4] == 'i' &&
                            line[currentPos+5] == 'd' &&
                            line[currentPos+6] == '(')
                            {
                                ++wordsCount[42];
                                currentPos += 5;
                            }
                    else if(line[currentPos+1] == 'y' &&
                            line[currentPos+2] == 'p' &&
                            line[currentPos+3] == 'e' &&
                            line[currentPos+4] == 'n' &&
                            line[currentPos+5] == 'a' &&
                            line[currentPos+6] == 'm' &&
                            line[currentPos+7] == 'e' &&
                            line[currentPos+8] == ' ')
                            {
                                ++wordsCount[43];
                                currentPos += 7;
                            }
                    break;
                case 'u':
                    if(line[currentPos+1] == 'n' &&
                        line[currentPos+2] == 's' &&
                        line[currentPos+3] == 'i' &&
                        line[currentPos+4] == 'g' &&
                        line[currentPos+5] == 'n' &&
                        line[currentPos+6] == 'e' &&
                        line[currentPos+7] == 'd' &&
                        line[currentPos+8] == ' ')
                        {
                            ++wordsCount[44];
                            currentPos += 7;
                        }
                    else if(line[currentPos+1] == 's' &&
                            line[currentPos+2] == 'i' &&
                            line[currentPos+3] == 'n' &&
                            line[currentPos+4] == 'g' &&
                            line[currentPos+5] == ' ')
                            {
                                ++wordsCount[45];
                                currentPos += 4;
                            }
                    break;
                case 'v':
                    if(line[currentPos+1] == 'i' &&
                        line[currentPos+2] == 'r' &&
                        line[currentPos+3] == 't' &&
                        line[currentPos+4] == 'u' &&
                        line[currentPos+5] == 'a' &&
                        line[currentPos+6] == 'l' &&
                        line[currentPos+7] == ' ')
                        {
                            ++wordsCount[46];
                            currentPos += 6;
                        }
                    else if(line[currentPos+1] == 'o' &&
                            line[currentPos+2] == 'i' &&
                            line[currentPos+3] == 'd' &&
                            line[currentPos+4] == ' ')
                            {
                                ++wordsCount[47];
                                currentPos += 3;
                            }
                    else if(line[currentPos+1] == 'o' &&
                            line[currentPos+2] == 'l' &&
                            line[currentPos+3] == 'a' &&
                            line[currentPos+4] == 't' &&
                            line[currentPos+5] == 'i' &&
                            line[currentPos+6] == 'l' &&
                            line[currentPos+7] == 'e' &&
                            line[currentPos+8] == ' ')
                            {
                                ++wordsCount[48];
                                currentPos += 7;
                            }
                    break;
                case 'w':
                    if(line[currentPos+1] == 'h' &&
                        line[currentPos+2] == 'i' &&
                        line[currentPos+3] == 'l' &&
                        line[currentPos+4] == 'e' &&
                        (line[currentPos+5] == '\0' || line[currentPos+5] == '\n' || line[currentPos+5] == '{'))
                        {
                            ++wordsCount[49];
                            currentPos += 4;
                        }
                    break;
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
    currentPos += (spacesNeeded); //if spaces were inserted or deleted, move the currentPos pointer accordingly
}

void reportResults(ostream* output, const string* reservedWords, const short unsigned* wordsCount)
{
    cout << "Reserved words: \tAmount of times used:" << endl;
    for(short unsigned i=0; i<50; ++i)
    {
        cout << reservedWords[i] << "\t\t\t" << wordsCount[i] << endl;
    }
}
