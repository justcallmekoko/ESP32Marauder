#include "EvilPortal.h"

EvilPortal::EvilPortal() {
  this->runServer = false;
  this->name_received = false;
  this->password_received = false;
}

String EvilPortal::get_user_name() {
  return this->user_name;
}

String EvilPortal::get_password() {
  return this->password;
}

void EvilPortal::startAP() {
  
}

void EvilPortal::startPortal() {
  // wait for flipper input to get config index
  this->startAP();

  this->runServer = true;
}