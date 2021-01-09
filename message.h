//
// Created by TYTY on 2021-01-03 003.
//

#ifndef BBCODE__MESSAGE_H_
#define BBCODE__MESSAGE_H_

#include <string>
#include <functional>

enum Severity {
  Tidy,
  Warning,
  Error
};

struct Message {
  Severity severity;

  usize line;
  usize chr;
  usize offset;
  usize span;

  std::string name;
  std::string message;
};

typedef std::function<void(Message&&)> MessageEmitter;

#endif //BBCODE__MESSAGE_H_
