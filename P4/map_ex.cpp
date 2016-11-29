// #include <iostream>
// #include <map>
//
// using namespace std;
// int main(){
//   std::map<int, std::string> name_map;
//
//   name_map[1] = "sneha";
//   name_map[2] = "snehal";
//   name_map[3] = "sweta";
//
//   std::map<int, std::string>::const_iterator itr;
//     for(itr = name_map.begin(); itr != name_map.end(); ++itr)
//     {
//         std::cout << itr->first << "=>" << itr->second << std::endl;
//     }
//
// //  for(std::map<int, std::string>::iterator it - name_map.begin(); it !=name_map.end();it++) {
// //    std::cout << it->first << "=>" << it->second << std::endl;
// //  }
//   return 0;
//
// }

#include <iostream>
#include <string>

using std::string;

string getFileName(const string& s) {

   char sep = '/';

   size_t i = s.rfind(sep, s.length());
   if (i != string::npos) {
      std::cout << "i \"" << i << "\"\n";
      //std::cout << "dir \"" << s.substr(0, i) << "\"\n";
      return(s.substr(i+1, s.length() - i));
   }

   return("");
}

string getDirName(const string& s) {

   char sep = '/';

   size_t i = s.rfind(sep, s.length());
   if (i != 0) {
      return(s.substr(0,i));
   }

   return("/");
}
int test(const char* path) {
  char *p = strdup(path);
}
int main(int argc, char** argv) {

   string path = argv[1];

   std::cout << "The file name is \"" << getFileName(path) << "\"\n";
   std::cout << "The dir name is \"" << getDirName(path) << "\"\n";
   string s ="abc";
   test(s);


}
