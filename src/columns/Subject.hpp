/*
 * Subject.hpp
 *
 *  Created on: Jul 30, 2016
 *      Author: pschultz
 */

#ifndef SUBJECT_HPP_
#define SUBJECT_HPP_

#include "columns/ObserverTable.hpp"
#include "columns/Messages.hpp"

namespace PV {

class Subject {
public:
   Subject() {}
   virtual ~Subject() {}
protected:
   void notify(ObserverTable const& table, std::vector<std::shared_ptr<BaseMessage> > messages);
   inline void notify(ObserverTable const& table, std::shared_ptr<BaseMessage> message) {
      notify(table, std::vector<std::shared_ptr<BaseMessage> >{message});
   }

};

} /* namespace PV */

#endif /* SUBJECT_HPP_ */
