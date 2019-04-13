/*
    Author: Mostafa Omar Mahmoud - 20170292
    Date: Apr. 13th 2019
    Version 1.1
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
#include <map>

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
    struct SecondaryIndex
    {
        string instructorName;
        short RRN; // RRN of the first record in the LabeLID file
        SecondaryIndex(string __instructorName, short __RRN)
        {
            instructorName = __instructorName;
            RRN = __RRN;
        }
        bool operator <(const SecondaryIndex &other) const{
            if (instructorName != other.instructorName)
                return instructorName < other.instructorName;
            return RRN < other.RRN;
        }
    };
    struct LabelID{
        char courseID[6];
        short next;
        LabelID(char __courseID[6] = "", short __next = 0)
        {
            strcpy(courseID, __courseID);
            next = __next;
        }
    };
    vector<PrimaryIndexRecord> vPrimaryIndex;
    vector<SecondaryIndex> vSecondaryIndex;
    vector<LabelID> vLabelID;
    string dataFilePath, primaryIndexFilePath, secondaryIndexFilePath, labelIDFilePath;
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
    void savePrimaryIndex()
    {
        ofstream fout(primaryIndexFilePath.c_str(), ios::binary);

        bool notUpToDate = false;
        fout.write((char*)&notUpToDate, sizeof(notUpToDate));

        for (auto r : vPrimaryIndex)
        {
            fout.write(r.courseID, sizeof(r.courseID));
            fout.write((char*)&r.offset, sizeof(r.offset));
        }
        fout.close();
    }

    int PrimaryKeyBinarySearch(char *courseID)
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

    int SecondaryKeyBinarySearch(const string &name)
    {
        int l = 0, r = (int)vSecondaryIndex.size() - 1;
        while (l <= r)
        {
            int mid = (l + r)/2;
            if (vSecondaryIndex[mid].instructorName == name)
                return mid;
            else if (name < vSecondaryIndex[mid].instructorName)
                r = mid - 1;
            else
                l = mid + 1;
        }
        return -1;
    }

    /*
     * loads secondary index in main memory
     */
    void loadSecondaryIndex()
    {
        vLabelID.clear();
        vSecondaryIndex.clear();
        fstream fin(dataFilePath.c_str(), ios::binary|ios::in|ios::out);
        Course crs;
        while(true)
        {
            if (!readCourse(fin, crs))
                break;
            if (crs.courseID[0] =='*')
                continue;
            short next = -1;

            int idx = SecondaryKeyBinarySearch(crs.instructorName);
            if (idx == -1)
            {
                SecondaryIndex r(crs.instructorName, vLabelID.size());
                vSecondaryIndex.push_back(r);
                sort(vSecondaryIndex.begin(), vSecondaryIndex.end());
            }
            else
            {
                next = vSecondaryIndex[idx].RRN;
                vSecondaryIndex[idx].RRN = vLabelID.size();
            }
            vLabelID.push_back(LabelID(crs.courseID, next));

        }
        fin.close();
    }
    void saveSecondaryIndex()
    {
        ofstream outSecondary(secondaryIndexFilePath.c_str(), ios::binary);
        ofstream outLabel(labelIDFilePath.c_str(), ios::binary);

        bool notUpToDate = false;
        outSecondary.write((char*)&notUpToDate, sizeof(notUpToDate));

        for (auto r : vSecondaryIndex)
        {
            if (r.RRN != -1)
            {
                outSecondary.write(r.instructorName.c_str(), r.instructorName.size());
                outSecondary.write((char *) &r.RRN, sizeof(r.RRN));
            }
        }
        outSecondary.close();

        for (auto r : vLabelID)
        {
            outLabel.write(r.courseID, sizeof(r.courseID));
            outLabel.write((char*)&r.next, sizeof(r.next));
        }
        outLabel.close();
    }


    void changeSecondaryIndexHeader(bool header)
    {
        fstream fout(secondaryIndexFilePath.c_str(), ios::out|ios::in|ios::binary);
        fout.write((char*)&header, sizeof(header));
        fout.close();
    }

    int isSecondaryUpToDate()
    {
        if (!exists(secondaryIndexFilePath))
            return -1;

        ifstream fin(secondaryIndexFilePath.c_str(), ios::binary);
        bool notUpToDateHeader;
        fin.read((char*)&notUpToDateHeader, sizeof(notUpToDateHeader));

        if (fin.fail() || fin.eof()) // file exists but empty
            return -2;

        fin.close();
        return !notUpToDateHeader;

    }

    void reconstructSecondaryIndex()
    {
        loadSecondaryIndex();
        saveSecondaryIndex();
    }

    void insertSecondary(Course &crs)
    {
        int idx = SecondaryKeyBinarySearch(crs.instructorName);
        int next = -1;
        if (idx == -1)
        {
            vSecondaryIndex.push_back(SecondaryIndex(crs.instructorName, vLabelID.size()));
            sort(vSecondaryIndex.begin(), vSecondaryIndex.end());
        }
        else
        {
            next = vSecondaryIndex[idx].RRN;
            vSecondaryIndex[idx].RRN = vLabelID.size();
        }
        vLabelID.push_back(LabelID(crs.courseID, next));
    }
public:
    CourseFileManager(string _dataFilePath = "defaultDataFile")
    {
        dataFilePath = _dataFilePath;
        primaryIndexFilePath = dataFilePath + "PrimaryIndex.txt";
        secondaryIndexFilePath = dataFilePath + "SecondaryIndex.txt";
        labelIDFilePath = dataFilePath + "LabelID.txt";
        dataFilePath += ".txt";

        createFile(dataFilePath);
        createFile(primaryIndexFilePath);
        createFile(secondaryIndexFilePath);
        createFile(labelIDFilePath);

        if (isPrimaryUpToDate() != 1)
        {
            reconstructPrimaryIndex();
            loadPrimaryIndex();
        }
        else
            loadPrimaryIndex();

        if (isSecondaryUpToDate() != 1)
        {
            reconstructSecondaryIndex();
        }
        else
            loadSecondaryIndex();
    }
    void addRecord(Course &crs)
    {
        changePrimaryIndexHeader(true);
        changeSecondaryIndexHeader(true);
        fstream fout(dataFilePath, ios::in|ios::out|ios::binary);
        fout.seekp(0, ios::end);
        int address = fout.tellp();
        addCourse(fout, crs);
        int i = 0;
        PrimaryIndexRecord r(crs.courseID, address);
        vPrimaryIndex.push_back(r);
        sort(vPrimaryIndex.begin(), vPrimaryIndex.end());

        insertSecondary(crs);
        fout.close();
    }
    /*
     * returns false if record is not found
     */
    bool deleteRecord(char courseID[])
    {
        int idx = PrimaryKeyBinarySearch(courseID);
        if (idx == -1)
            return false;

        changePrimaryIndexHeader(true);

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
    bool updateRecord(char courseID[], Course &crs, bool keepID = false)
    {
        if (!deleteRecord(courseID))
            return false;

        changePrimaryIndexHeader(true);
        if (keepID)
            strcpy(crs.courseID, courseID);
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
        int idx = PrimaryKeyBinarySearch(courseID);
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
        savePrimaryIndex();
        saveSecondaryIndex();
    }

    bool printByInstructorName(const string &name)
    {
        int idx = SecondaryKeyBinarySearch(name);
        if (idx == -1)
            return 0;
        for (int k = vSecondaryIndex[idx].RRN; k != -1; k = vLabelID[k].next)
            printByCourseID(vLabelID[k].courseID);
        return 1;
    }

    bool updateRecordByInstructorName(const string &name, Course &crs)
    {
        int idx = SecondaryKeyBinarySearch(name);
        if (idx == -1)
            return 0;
        for (int k = vSecondaryIndex[idx].RRN; k != -1; k = vLabelID[k].next)
            updateRecord(vLabelID[k].courseID, crs);
        return 1;
    }

    bool deleteRecordByInstructorName(const string &name)
    {
        int idx = SecondaryKeyBinarySearch(name);
        if (idx == -1)
            return 0;
        for (int k = vSecondaryIndex[idx].RRN; k != -1; k = vLabelID[k].next)
            deleteRecord(vLabelID[k].courseID);
        return 1;

    }
};

void printMenu()
{
    cout << "\nWhat would you like to do now?\n\n"
            "1- Print All Courses\n"
            "2- Add Course\n"
            "3- Delete Course (by Course ID)\n"
            "4- Update Course (by Course ID\n"
            "5- Print Course (by Course ID)\n"
            "6- Delete Course (by instructor name)\n"
            "7- Update Course (by instructor name)\n"
            "8- Print Course (by instructor name)\n"
            "9- Close current file\n"
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
                    string name;
                    cout << "Enter instructor name: ";
                    cin.ignore(32767, '\n');
                    getline(cin, name);
                    manager->deleteRecordByInstructorName(name);
                    break;
                }
                case 7:
                {
                    string name;
                    cout << "Enter instructor name: ";
                    cin.ignore(32767, '\n');
                    getline(cin, name);
                    Course crs;
                    cout << "Enter new course: \n";
                    cin >> crs;
                    manager->updateRecordByInstructorName(name, crs);
                    break;
                }
                case 8:
                {
                    string name;
                    cout << "Enter instructor name: ";
                    cin.ignore(32767, '\n');
                    getline(cin, name);
                    manager->printByInstructorName(name);
                    break;
                }
                case 9:
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