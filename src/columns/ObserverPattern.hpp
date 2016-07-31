/*
 * ObserverPattern.hpp
 *
 *  Created on: Jul 30, 2016
 *      Author: pschultz
 */

#ifndef SRC_COLUMNS_OBSERVERPATTERN_HPP_
#define SRC_COLUMNS_OBSERVERPATTERN_HPP_

#include <columns/ObserverTable.hpp>
#include "columns/BaseObject.hpp"

namespace PV {

class ObserverPattern {
public:
   ObserverPattern();
   virtual ~ObserverPattern();
protected:
   void notify(ObjectHierarchy const& recipients, std::vector<std::shared_ptr<BaseMessage> > messages);
   inline void notify(ObjectHierarchy const& recipients, std::shared_ptr<BaseMessage> message) {
      notify(recipients, std::vector<std::shared_ptr<BaseMessage> >{message});
   }
};

} /* namespace PV */

#endif /* SRC_COLUMNS_OBSERVERPATTERN_HPP_ */
