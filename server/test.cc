
#include <iostream>
#include <cstring>
#include <map>

using namespace std;

int main(void)
{
    map<const char[], int> m;
    char c[12] = "99999";
    char b[15] = "99999";
    //cout << sizeof(c) << endl;
    //cout << strlen(c) << endl;
    m.insert(make_pair(c, 10));
    auto it = m.find("99999");
    if (it != m.end()) {
        cout << it->first << it->second << endl;
    }
    cout << m[b] << endl;
    return 0;
}
