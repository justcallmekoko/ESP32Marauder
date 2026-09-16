#pragma once

template <typename ListType>
ListType* resetOwnedList(ListType*& list) {
  if (list != nullptr) {
    list->clear();
    delete list;
  }

  list = new ListType();
  return list;
}
