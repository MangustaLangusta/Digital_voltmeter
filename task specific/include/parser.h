#ifndef PARSER_H
#define PARSER_H

#include <list>
#include <string>

namespace parser{
  
bool ParseMessage(std::string source_str, const std::string delimiter, std::list<std::string> *parsed_list){
  
  parsed_list->clear();
  
  if(source_str.empty())
    return false;
  
  const int delim_len = delimiter.length();
  
  if(delim_len == 0)
    return false;
  
  while(!source_str.empty()){
    std::string::size_type delim_pos = source_str.find(delimiter);
    std::string new_elem = source_str.substr(0, delim_pos);
    
    if(new_elem.length() > 0)
      parsed_list->push_back(new_elem);
    
    if(delim_pos == std::string::npos)
      break;
    
    source_str.erase(0, delim_pos + delim_len);
  }
  
  return true;
}
  
  
  
}


#endif          //PARSER_H