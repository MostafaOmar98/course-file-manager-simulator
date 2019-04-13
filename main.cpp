/*
    Author: Mostafa Omar Mahmoud - 20170292
    Date: Apr. 13th 2019
    Version 1.0
    Fields & Record organization with File Structures using C++
    Manages 'Course' records with the following attributes:
    Course ID (Primary Key)
    Course Name
    Instructor Name (Secondary Key)
    Number of weeks

    Method of Organization: Length indicator records, delimeted fields

    Operations:
     Print All Courses
     Add Course Delete Course
     (by Course ID) Update Course
     (by Course ID Print Course (by Course ID)
     Delete Course (by instructor name)
     Update Course (by instructor name) Print Course (by instructor name)
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>

using namespace std;


const char fieldDelimeter = '|';
struct Course{
    char courseID[6];
    string courseName, instructorName;
    short weeks;
};

istream& operator>> (istream &in, Course &crs)
{
    cout << "Enter Course ID (5 chars): ";
    in >> crs.courseID;
    in.ignore();
    cout << "Enter Course Name: ";
    getline(in, crs.courseName); // Don't forget to handle ignores
    cout << "Enter Instructor Name: ";
    getline(in, crs.instructorName);
    cout << "Enter Number of Weeks: ";
    in >> crs.weeks;
    return in;
}
ostream& operator<< (ostream &out, const Course &crs)
{
    out << "Course ID: " << crs.courseID << '\n';
    out << "Course Name: " << crs.courseName << '\n';
    out << "Instructor Name: " << crs.instructorName << '\n';
    out << "Number of Weeks: " << crs.weeks << '\n';
}

void addCourse(fstream &fout, const Course &crs)
{
    string buffer;
    buffer.append(crs.courseID);
    buffer += fieldDelimeter;
    buffer += crs.courseName;
    buffer += fieldDelimeter;
    buffer += crs.instructorName;
    buffer += fieldDelimeter;
    short recordSize = buffer.size() + sizeof(crs.weeks) + sizeof(fieldDelimeter);
    fout.write((char*)&recordSize, sizeof(recordSize));
    fout.write(buffer.c_str(), buffer.size());
    fout.write((char*)&crs.weeks, sizeof(crs.weeks));
    fout.put(fieldDelimeter);
}

/*
 * Reads a course record and returns its size
 * returns 0 in case of failure of reading
 */
short readCourse(fstream &fin, Course &crs)
{
    short recordSize;
    fin.read((char*)&recordSize, sizeof(recordSize));
    if (fin.fail() || fin.eof())
        return 0;
    // recordSizecourseID|courseName|instructorName|weeks|
    fin.getline(crs.courseID, 6, fieldDelimeter);
    getline(fin, crs.courseName, fieldDelimeter);
    getline(fin, crs.instructorName, fieldDelimeter);
    fin.read((char*)&crs.weeks, sizeof(crs.weeks));
    fin.ignore();
    return recordSize;
}



bool exists(const string &filePath)
{
    fstream myFile(filePath.c_str(), ios::binary|ios::in|ios::out);
    if (myFile.fail())
        return false;
    else
    {
        myFile.close();
        return true;
    }
}
/*
 * returns false if file already exists
 * Creates a file and returns true otherwise
 */
bool createFile(const string &filePath)
{
    if (exists(filePath))
        return false;
    ofstream fout(filePath, ios::binary);
    fout.close();
    return true;
}

/*
 * File Manager for data of type Course
 * Method of Organization: Length indicator records, delimeted fields
 */
class CourseFileManager
{
private:
    struct PrimaryIndexRecord
    {
        char courseID[6];
        int offset; // OFFSET IS INT CAREFUL
        PrimaryIndexRecord(char __courseID[6] = "", int __offset = 0)
        {
            strcpy(courseID, __courseID);
            offset = __offset;
        }
        bool operator< (const PrimaryIndexRecord &other) const{
            short cmp = strcmp(courseID, other.courseID);
            if (cmp != 0)
                return cmp < 1;
            return offset < other.offset;
        }
    };
    vector<PrimaryIndexRecord> vPrimaryIndex;
    string dataFilePath, primaryIndexFilePath;
    /*
     * Checks if primary index file corresponds to the current data file
     * Returns 0 if it's not up to date, -1 if it doesn't exist, -2 if it exists but empty, 1 if it is up to date
     */
    int isPrimaryUpToDate()
    {
        if (!exists(primaryIndexFilePath))
            return -1;

        ifstream fin(primaryIndexFilePath.c_str(), ios::binary);
        bool notUpToDateHeader;
        fin.read((char*)&notUpToDateHeader, sizeof(notUpToDateHeader));

        if (fin.fail() || fin.eof()) // file exists but empty
            return -2;

        fin.close();
        return !notUpToDateHeader;
    }
    void changePrimaryIndexHeader(bool state)
    {
        fstream fout(primaryIndexFilePath, ios::in|ios::out|ios::binary);
        fout.write((char*)&state, sizeof(state));
        fout.close();
    }
    /*
     * constructs a primary index file from the data file
     * NOTES:
     *  You should handle creating the file before calling this function
     *  You should load primary index into main memory after calling this function
     */
    void reconstructPrimaryIndex()
    {
        ofstream fout(primaryIndexFilePath.c_str(), ios::binary);
        fstream fin(dataFilePath.c_str(), ios::binary|ios::in|ios::out);
        bool notUpToDateHeader = false;
        fout.write((char*)&notUpToDateHeader, sizeof(notUpToDateHeader));

        Course crs;
        while(true)
        {
            int address = fin.tellg();
            short recordSize = readCourse(fin, crs);
            if (recordSize == 0)
                break;
            if (crs.courseID[0] != '*')
            {
                fout.write(crs.courseID, sizeof(crs.courseID));
                fout.write((char*)&address, sizeof(address));
            }
        }
        fout.close();
        fin.close();
    }
    /*
     * Loads primary index file in memory
     * NOTE: You should handle whether primary index file is up to date or not before calling this function
     */
    void loadPrimaryIndex()
    {
        vPrimaryIndex.clear();
        ifstream fin(primaryIndexFilePath.c_str(), ios::binary);
        fin.seekg(1, ios::beg); // skips header
        PrimaryIndexRecord temp;
        while(true)
        {
            fin.read(temp.courseID, sizeof(temp.courseID));
            if (fin.fail() || fin.eof())
                break;
            fin.read((char*)&temp.offset, sizeof(temp.offset));
            vPrimaryIndex.push_back(temp);
        }
        fin.close();
        sort(vPrimaryIndex.begin(), vPrimaryIndex.end());
    }
public:
    CourseFileManager(string _dataFilePath = "defaultDataFile", string _primaryIndexFilePath = "")
    {
        dataFilePath = _dataFilePath;
        primaryIndexFilePath = dataFilePath + "PrimaryIndex.txt";
        dataFilePath += ".txt";

        createFile(dataFilePath);
        createFile(primaryIndexFilePath);

        if (isPrimaryUpToDate() != 1)
        {
            reconstructPrimaryIndex();
            loadPrimaryIndex();
        }
        else
            loadPrimaryIndex();
    }
    void addRecord(Course &crs)
    {
        changePrimaryIndexHeader(true);

        fstream fout(dataFilePath, ios::in|ios::out|ios::binary);
        fout.seekp(0, ios::end);
        int address = fout.tellp();
        addCourse(fout, crs);
        int i = 0;
        PrimaryIndexRecord r(crs.courseID, address);
        vPrimaryIndex.push_back(r);
        sort(vPrimaryIndex.begin(), vPrimaryIndex.end());
        fout.close();
    }
    int binarySearch(char courseID[])
    {
        int l = 0, r = (int)vPrimaryIndex.size() - 1;
        while(l <= r)
        {
            int mid = (l + r)/2;
            short cmp = strcmp(courseID, vPrimaryIndex[mid].courseID);
            if (cmp == 0)
                return mid;
            else if (cmp == -1)
                r = mid - 1;
            else
                l = mid + 1;
        }
        return -1;
    }
    /*
     * returns false if record is not found
     */
    bool deleteRecord(char courseID[])
    {
        int idx = binarySearch(courseID);
        if (idx == -1)
            return false;

        int address = vPrimaryIndex[idx].offset;
        fstream fout(dataFilePath, ios::in|ios::out|ios::binary);
        fout.seekp(address + 2, ios::beg); // skips record length and adds '*' to the beginning of courseID
        fout.put('*');

        vPrimaryIndex.erase(vPrimaryIndex.begin() + idx);
        fout.close();
        return true;
    }
    /*
     * returns false if record is not found
     */
    bool updateRecord(char courseID[], Course &crs)
    {
        if (!deleteRecord(courseID))
            return false;
        addRecord(crs);
        return true;
    }
    void printAll()
    {
        fstream fin(dataFilePath, ios::in|ios::out|ios::binary);
        Course crs;
        int i = 0;
        while(true)
        {
            if (!readCourse(fin, crs))
                break;
            if (crs.courseID[0] == '*')
                continue;
            cout << "Course #" << ++i << ":\n";
            cout << crs << '\n';
        }
        fin.close();
    }
    bool printByCourseID(char courseID[])
    {
        int idx = binarySearch(courseID);
        if (idx == -1)
            return false;
        int address = vPrimaryIndex[idx].offset;
        fstream fin(dataFilePath, ios::in|ios::out|ios::binary);
        fin.seekg(address, ios::beg);
        Course crs;
        readCourse(fin, crs);
        cout << crs << '\n';
        return true;
    }
    ~CourseFileManager()
    {
        //saves primary index file
        reconstructPrimaryIndex();
    }
};

void printMenu()
{
    cout << "\nWhat would you like to do now?\n\n"
            "1- Print All Courses\n"
            "2- Add Course\n"
            "3- Delete Course (by Course ID)\n"
            "4- Update Course (by Course ID\n"
            "5- Print (by Course ID)\n"
            "6- Close current file\n"
            "0- Exit\n\n";
}

string askForFilePath()
{
    cout << "Please Enter File Path: ";
    string filePath;
    cin >> filePath;
    return filePath;
}

int main()
{
    CourseFileManager* manager = 0;
    bool done = false;
    while(!done)
    {
        if (manager == 0)
        {
            string filePath = askForFilePath();
            manager = new CourseFileManager(filePath);
        }
        else
        {
            printMenu();
            int choice;
            cout << "Enter choice: ";
            cin >> choice;
            switch (choice)
            {
                case 0:
                {
                    done = true;
                    delete manager;
                    break;
                }
                case 1:
                {
                    manager->printAll();
                    break;
                }
                case 2:
                {
                    Course crs;
                    cin >> crs;
                    manager->addRecord(crs);
                    cout << "Record added!\n";
                    break;
                }
                case 3:
                {
                    char courseID[6];
                    cout << "Enter course ID to delete: ";
                    cin >> courseID;

                    if (manager->deleteRecord(courseID))
                        cout << "Course Deleted!\n";
                    else
                        cout << "Course not found!\n";
                    break;
                }
                case 4:
                {
                    char courseID[6];
                    cout << "Enter course ID to update: ";
                    cin >> courseID;
                    cout << "Enter new course: \n";
                    Course crs;
                    cin >> crs;
                    if (manager->updateRecord(courseID, crs))
                        cout << "Course updated!\n";
                    else
                        cout << "Course not found!\n";
                    break;
                }
                case 5:
                {
                    char courseID[6];
                    cout << "Enter course ID to print: ";
                    cin >> courseID;
                    if (!manager->printByCourseID(courseID))
                        cout << "Course not found!\n";
                    break;
                }
                case 6:
                {
                    delete manager;
                    manager = 0;
                    cout << "File closed!\n";
                    break;
                }
                default:
                {
                    cout << "Invalid Choice!\n";
                    break;
                }
            }
        }
    }
    return 0;
}