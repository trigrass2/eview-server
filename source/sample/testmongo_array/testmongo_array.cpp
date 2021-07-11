// testmongo_array.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
/*
#include <iostream>
#include <cstdlib>
#include <winsock2.h>
#include "mongo\client\dbclient.h"
using namespace std;
void run() {
	mongo::DBClientConnection c;
	c.connect("localhost:17001");
}
int main() {
	mongo::client::initialize();
	try {
		run();
		std::cout << "connected ok" << std::endl;
	}
	catch (const mongo::DBException &e) {
		std::cout << "caught " << e.what() << std::endl;
	}
	return 1;
}
*/

#include <ace/Task.h>
#include "ace/Reactor.h"
#include "ace/Select_Reactor.h"
#include <iostream>
#include <cstdlib>
#include <winsock2.h>
#include "mongo/client/dbclient.h"

#include <iostream>
#include <list>
#include <vector>

using mongo::BSONArray;
using mongo::BSONArrayBuilder;
using mongo::BSONObj;
using mongo::BSONObjBuilder;
using mongo::BSONElement;
using std::cout;
using std::endl;
using std::list;
using std::vector;

int main(int argc, char** argv) {
	ACE_Select_Reactor *pSelectReactor = new ACE_Select_Reactor();
	ACE_Reactor* m_pReactor = new ACE_Reactor(pSelectReactor, true);

	mongo::client::GlobalInstance instance;
	if (!instance.initialized()) {
		std::cout << "failed to initialize the client driver: " << instance.status() << endl;
		return EXIT_FAILURE;
	}

	//this->reactor(m_pReactor);

	// Build an object
	BSONObjBuilder bob;

	// Build an array
	BSONArrayBuilder bab;
	bab.append("first");
	bab.append("second");
	bab.append("third");
	bab.append("fourth").append("fifth");

	// Place array in object at key "x"
	bob.appendArray("x", bab.arr());

	// Use BSON_ARRAY macro like BSON macro, but without keys
	BSONArray arr = BSON_ARRAY("hello" << 1 << BSON("foo" << BSON_ARRAY("bar" << "baz" << "qux"))<<BSON("vs"<<BSON("v"<<"abcd")));
	// Place the second array in object at key "y"
	bob.appendArray("y", arr);

	// Create the object
	BSONObj an_obj = bob.obj();


	vector<BSONElement> elements = an_obj["x"].Array();

	// Print the array out
	cout << "Our Array:" << endl;
	for (vector<BSONElement>::iterator it = elements.begin(); it != elements.end(); ++it) {
		cout << *it << endl;
	}
	cout << endl;

	// Extract the array as a BSONObj
	BSONObj myarray = an_obj["y"].Obj();

	// Convert it to a vector
	vector<BSONElement> v;
	myarray.elems(v);

	// Convert it to a list
	list<BSONElement> L;
	myarray.elems(L);

	// Print the vector out
	BSONElement elem = v[3];
	BSONObj obj = elem.Obj();
	obj.dump();
	obj["vs"];
	obj["vs"]["v"];
	cout << obj["vs"]["v"];
	cout << "The Vector Version:" << endl;
	int i = -1;
	for (vector<BSONElement>::iterator it = v.begin(); it != v.end(); ++it) {
		i++;
		if (i == 3)
		{
			cout << obj["vs"]["v"];
			cout << *it << endl;
		}
		cout << *it << endl;
	}

	cout << endl;

	// Print the list out
	cout << "The List Version:" << endl;
	for (list<BSONElement>::iterator it = L.begin(); it != L.end(); ++it) {
		cout << *it << endl;
	}
}
