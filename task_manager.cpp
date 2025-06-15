#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <sstream>

using namespace std;

// Structure to represent a user
struct User {
    string name;
    int groupId;
    bool isLeader;
    int numTasks;
    vector<string> tasks;
    vector<string> completedTasks;
    map<string, string> taskCompletionTime;
    map<string, chrono::high_resolution_clock::time_point> taskStartTimes;
};

// Structure to represent a group
struct Group {
    int id;
    string name;
    vector<User> users;
};

// Global containers
map<int, Group> groups;
vector<User> userList;
// Save all data to file (called after updates)
void saveDataToFile() {
    ofstream file("data.txt");
    for (map<int, Group>::iterator it = groups.begin(); it != groups.end(); ++it) {
        int gid = it->first;
        Group& group = it->second;
        file << "Group " << gid << ": " << group.name << "\n";
        for (size_t i = 0; i < group.users.size(); ++i) {
            User& user = group.users[i];
            file << "  - " << user.name << " " << user.groupId << " "
                 << user.isLeader << " " << user.numTasks << "\n";
            for (size_t j = 0; j < user.tasks.size(); ++j)
                file << "    * " << user.tasks[j] << "\n";
            for (size_t j = 0; j < user.completedTasks.size(); ++j) {
                string task = user.completedTasks[j];
                file << "    # " << task << " | " << user.taskCompletionTime[task] << "\n";
            }
        }
    }
    file.close();
}

// Load data from file at start
void loadDataFromFile() {
    ifstream file("data.txt");
    if (!file.is_open()) return;

    string line;
    int currentGroupId = -1;
    User tempUser;

    while (getline(file, line)) {
        if (line.find("Group ") == 0) {
            size_t colon = line.find(":");
            currentGroupId = stoi(line.substr(6, colon - 6));
            string gname = line.substr(colon + 2);
            Group g = {currentGroupId, gname, {}};
            groups[currentGroupId] = g;
        } else if (line.find("  - ") == 0) {
            istringstream ss(line.substr(4));
            ss >> tempUser.name >> tempUser.groupId >> tempUser.isLeader >> tempUser.numTasks;
            tempUser.tasks.clear();
            tempUser.completedTasks.clear();
            tempUser.taskStartTimes.clear();
            tempUser.taskCompletionTime.clear();
            groups[currentGroupId].users.push_back(tempUser);
            userList.push_back(tempUser);
        } else if (line.find("    * ") == 0) {
            string task = line.substr(6);
            groups[currentGroupId].users.back().tasks.push_back(task);
            groups[currentGroupId].users.back().numTasks++;
        } else if (line.find("    # ") == 0) {
            size_t sep = line.find("|");
            string task = line.substr(6, sep - 7);
            string duration = line.substr(sep + 2);
            groups[currentGroupId].users.back().completedTasks.push_back(task);
            groups[currentGroupId].users.back().taskCompletionTime[task] = duration;
        }
    }

    file.close();
}

// Update a user in the global user list
void updateUserInList(const User& updatedUser) {
    for (size_t i = 0; i < userList.size(); ++i) {
        if (userList[i].name == updatedUser.name && userList[i].groupId == updatedUser.groupId) {
            userList[i] = updatedUser;
            break;
        }
    }
}
// Create a new group
void createGroup(int groupId, const string& groupName) {
    if (groups.find(groupId) == groups.end()) {
        Group g = {groupId, groupName, {}};
        groups[groupId] = g;
    }
}

// Add a user to a group
void addUser(int groupId, string name, bool isLeader) {
    vector<User>& users = groups[groupId].users;
    for (size_t i = 0; i < users.size(); ++i) {
        if (users[i].name == name) {
            cout << "User already exists in this group.\n";
            return;
        }
    }
    User user = {name, groupId, isLeader, 0, {}, {}, {}, {}};
    groups[groupId].users.push_back(user);
    userList.push_back(user);
    saveDataToFile();
    cout << "User " << name << " added successfully.\n";
}

// Assign a task to a user
void assignTask(int groupId, string taskName, string userName) {
    vector<User>& users = groups[groupId].users;
    for (size_t i = 0; i < users.size(); ++i) {
        if (users[i].name == userName) {
            users[i].tasks.push_back(taskName);
            users[i].numTasks++;
            users[i].taskStartTimes[taskName] = chrono::high_resolution_clock::now();
            updateUserInList(users[i]);
            saveDataToFile();
            cout << "Task " << taskName << " assigned to " << userName << ".\n";
            return;
        }
    }
    cout << "User not found in group " << groupId << ".\n";
}

// Complete a task and log time taken
void completeTask(int groupId, string taskName, string userName) {
    vector<User>& users = groups[groupId].users;
    for (size_t i = 0; i < users.size(); ++i) {
        if (users[i].name == userName) {
            vector<string>& tasks = users[i].tasks;
            for (size_t j = 0; j < tasks.size(); ++j) {
                if (tasks[j] == taskName) {
                    auto endTime = chrono::high_resolution_clock::now();
                    auto startTime = users[i].taskStartTimes[taskName];
                    long long duration = chrono::duration_cast<chrono::seconds>(endTime - startTime).count();
                    int hours = duration / 3600;
                    int minutes = (duration % 3600) / 60;
                    int seconds = duration % 60;
                    ostringstream oss;
                    oss << setfill('0') << setw(2) << hours << "h "
                        << setw(2) << minutes << "m "
                        << setw(2) << seconds << "s";

                    users[i].taskCompletionTime[taskName] = oss.str();
                    users[i].completedTasks.push_back(taskName);
                    tasks.erase(tasks.begin() + j);
                    users[i].taskStartTimes.erase(taskName);
                    users[i].numTasks--;
                    updateUserInList(users[i]);
                    saveDataToFile();
                    cout << "Task '" << taskName << "' completed by " << userName << " in " << oss.str() << ".\n";
                    return;
                }
            }
        }
    }
    cout << "Task not found for user.\n";
}

// Suggest the next available user
string suggestUser(int groupId) {
    sort(userList.begin(), userList.end(), [](const User& a, const User& b) {
        return a.numTasks < b.numTasks;
    });
    for (size_t i = 0; i < userList.size(); ++i) {
        if (userList[i].groupId == groupId)
            return userList[i].name;
    }
    return "No users available in this group";
}
// View all tasks and history for a specific user
void viewUserSummary(string userName, int groupId) {
    const vector<User>& users = groups[groupId].users;
    for (size_t i = 0; i < users.size(); ++i) {
        if (users[i].name == userName) {
            cout << "\nActive Tasks:\n";
            for (size_t j = 0; j < users[i].tasks.size(); ++j)
                cout << " - " << users[i].tasks[j] << endl;

            cout << "\nCompleted Tasks:\n";
            for (size_t j = 0; j < users[i].completedTasks.size(); ++j) {
                string task = users[i].completedTasks[j];
                cout << " - " << task << " (Time Taken: " << users[i].taskCompletionTime.at(task) << ")\n";
            }
            return;
        }
    }
    cout << "User not found.\n";
}

// View all users in a group with task stats
void viewGroupUsers(int groupId) {
    if (groups.find(groupId) == groups.end()) {
        cout << "Group not found.\n";
        return;
    }
    Group& group = groups[groupId];
    cout << "\nUsers in Group '" << group.name << "':\n";
    cout << "Total Users: " << group.users.size() << "\n";

    for (size_t i = 0; i < group.users.size(); ++i) {
        cout << " - " << group.users[i].name;
        if (group.users[i].isLeader) cout << " (Leader)";
        cout << " | Tasks assigned: " << group.users[i].numTasks << "\n";
    }
}

// 🚦 Main menu loop
int main() {
    loadDataFromFile();

    int numGroups;
    cout << "Enter number of groups: ";
    cin >> numGroups;
    cin.ignore();

    for (int i = 0; i < numGroups; ++i) {
        int id;
        string name;
        cout << "Group ID and Name: ";
        cin >> id;
        cin.ignore();
        getline(cin, name);
        createGroup(id, name);
    }

    while (true) {
        cout << "\nMenu:\n"
             << "1. Add user\n"
             << "2. Assign task\n"
             << "3. Suggest user\n"
             << "4. Complete task\n"
             << "5. View user summary\n"
             << "6. View group users\n"
             << "7. Exit\n";

        int option;
        cout << "Choice: ";
        cin >> option;

        if (option == 1) {
            int gid;
            string uname;
            bool leader;
            cout << "Group ID: ";
            cin >> gid;
            cout << "Name: ";
            cin.ignore();
            getline(cin, uname);
            cout << "Is Leader (1/0): ";
            cin >> leader;
            addUser(gid, uname, leader);
        } else if (option == 2) {
            int gid;
            string task, user;
            cout << "Group ID: ";
            cin >> gid;
            cout << "Task name: ";
            cin.ignore();
            getline(cin, task);
            cout << "User name: ";
            getline(cin, user);
            assignTask(gid, task, user);
        } else if (option == 3) {
            int gid;
            cout << "Group ID: ";
            cin >> gid;
            cout << "Suggested user: " << suggestUser(gid) << "\n";
        } else if (option == 4) {
            int gid;
            string task, user;
            cout << "Group ID: ";
            cin >> gid;
            cout << "Task name: ";
            cin.ignore();
            getline(cin, task);
            cout << "User name: ";
            getline(cin, user);
            completeTask(gid, task, user);
        } else if (option == 5) {
            int gid;
            string uname;
            cout << "Group ID: ";
            cin >> gid;
            cout << "User name: ";
            cin.ignore();
            getline(cin, uname);
            viewUserSummary(uname, gid);
        } else if (option == 6) {
            int gid;
            cout << "Group ID: ";
            cin >> gid;
            viewGroupUsers(gid);
        } else if (option == 7) {
            cout << "Exiting. All data saved.\n";
            break;
        } else {
            cout << "Invalid option. Try again.\n";
        }
    }

    return 0;
}