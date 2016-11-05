#include <iostream>
#include <memory>
#include <scopegraph/Agent.h>
#include <scopegraph/Scope.h>

class HelloSignal : public sg::Signal {};

class HelloAgent : public sg::Agent<HelloAgent, sg::Provides<HelloSignal>> {

public:

	void saySomething() {

		send<HelloSignal>();
	}
};

class Listener : public sg::Agent<Listener, sg::Accepts<HelloSignal>> {

public:

	void onSignal(HelloSignal& signal) {

		std::cout << "received HelloSignal" << std::endl;
	}
};

class Scope : public sg::Scope<Scope> {};

int main(int argc, char** argv) {

	auto scope      = std::make_shared<Scope>();
	auto helloAgent = std::make_shared<HelloAgent>();
	auto listener   = std::make_shared<Listener>();

	scope->add(helloAgent);
	scope->add(listener);

	helloAgent->saySomething();
}
