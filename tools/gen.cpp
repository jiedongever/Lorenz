/*--------------------------------------------------------------------------*/
/*  Copyright 2016 Sergei Vostokin                                          */
/*                                                                          */
/*  Licensed under the Apache License, Version 2.0 (the "License");         */
/*  you may not use this file except in compliance with the License.        */
/*  You may obtain a copy of the License at                                 */
/*                                                                          */
/*  http://www.apache.org/licenses/LICENSE-2.0                              */
/*                                                                          */
/*  Unless required by applicable law or agreed to in writing, software     */
/*  distributed under the License is distributed on an "AS IS" BASIS,       */
/*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*/
/*  See the License for the specific language governing permissions and     */
/*  limitations under the License.                                          */
/*--------------------------------------------------------------------------*/

#include <string>
#include <list>
#include <iostream>
#include <fstream>
#include <locale>
#include <cstdlib>

using namespace std;

struct submessage{
  string name;
  bool dummy;
  enum {CALL,ANSWER} type;
};

struct message{
  string name;
  bool duplex;
  bool serilizable;
};

struct port{
  string name;
  string message_name;
  enum {SERVER,CLIENT,TASK} type;
};

struct actor{
  string name;
  bool serilizable;
  bool initially_active;
  list<port> ports;
};

// from parse.cpp
bool openparse(string&name,string&pragma);
bool getpragma(string&argstring,int&line);
void closeparse();

// from lexer.cpp
void lexinit(string&s);
bool getlex(string&lex);
bool ungetlex();

void error(int line)
{
	cerr << "ERROR: bad #pragma at line " << line << endl;
	exit(EXIT_FAILURE);
}

bool is_id(string&s)
{
	locale loc;
	return s.length()>1 || (s.length()==1 && isalpha(s[0],loc));
}

bool parse_message(int line, message& m)
{
	string lex;
	
	if (!(getlex(lex) && lex == "~")){ ungetlex(); return false; }

	if (!(getlex(lex) && is_id(lex))) error(line);

	m.name = lex;
	
	if (getlex(lex) && lex == "$"){ m.serilizable = true; }
	else{ ungetlex(); m.serilizable = false; }

	if (getlex(lex) && lex == "="){ m.duplex = true; }
	else{ ungetlex(); m.duplex = false; }

	if (getlex(lex)) error(line);

	return true;
}

bool parse_actor(int line, actor& a)
{
	string lex;

	if (!(getlex(lex) && lex == "*")){ ungetlex(); return false; }

	if (!(getlex(lex) && is_id(lex))) error(line);

	a.name = lex;

	if (getlex(lex) && lex == "$"){ a.serilizable = true; }
	else{ ungetlex(); a.serilizable = false; }

	if (getlex(lex) && lex == "("){

		port p;

		if (!(getlex(lex) && is_id(lex))) error(line);

		p.name = lex;

		if (getlex(lex) && lex == "?"){ p.type = port::SERVER; }
		else if (ungetlex() && getlex(lex) && lex == "!"){ p.type = port::CLIENT; }
		else error(line);

		if (!(getlex(lex) && is_id(lex))) error(line);

		p.message_name = lex;

		a.ports.push_back(p);

		while (getlex(lex) && lex == ","){

			if (!(getlex(lex) && is_id(lex))) error(line);

			p.name = lex;

			if (getlex(lex) && lex == "?"){ p.type = port::SERVER; }
			else if (ungetlex() && getlex(lex) && lex == "!"){ p.type = port::CLIENT; }
			else if (ungetlex() && getlex(lex) && lex == ".") { p.type = port::TASK; }
			else error(line);

			if (!(getlex(lex) && is_id(lex))) error(line);

			p.message_name = lex;

			a.ports.push_back(p);
		}
		ungetlex();
		if (!(getlex(lex) && lex == ")")) error(line);
	}

	if (!a.ports.size()) ungetlex();

	if (getlex(lex) && lex == "+"){ a.initially_active = true; }
	else{ ungetlex(); a.initially_active = false; }

	if (getlex(lex)) error(line);

	return true;
}

void print_message(ostream& s, message& m)
{
	bool comma = false;

	s << "~" << m.name;
	if (m.serilizable) s << "$";

	if (m.duplex) s << "=";
}

void print_actor(ostream&s,actor&a)
{
	bool comma = false;

	s << "*" << a.name;
	if (a.serilizable) s << "$";

	if (a.ports.size()){
		s << "(";

		for(std::list<port>::iterator it = a.ports.begin(); it!= a.ports.end();it++){
			port& x = *it;

			if (comma) s << ",";

			s << x.name;
			if (x.type == port::CLIENT) s << "!";
			else if (x.type == port::SERVER) s << "?";
			else if (x.type == port::TASK) s << ".";
			s << x.message_name;

			comma = true;
		}
		s << ")";
	}

	if (a.initially_active) s << "+";
}

void design(ofstream&outf, list<message>&mlist, list<actor>&alist)
{
	outf << "/*$TET$actor*/\n"
		"#include <templet.hpp>\n"
		"/*$TET$*/\n";

	outf << endl;

	outf << "using namespace TEMPLET;\n";

	outf << endl;

	for(std::list<message>::iterator it=mlist.begin();it!=mlist.end();it++){
		message& m = *it;

		outf << "#pragma templet "; print_message(outf, m); outf << endl << endl;
		outf << "struct " << m.name << " : message_interface{\n";

		if(!m.duplex)outf << "\t" << m.name << "(actor*, engine*);\n";

		if (m.serilizable){
			outf << "\tvoid save(saver*s){\n"
				"/*$TET$" << m.name << "$$save*/\n"
				"\t\t//::save(s, &x, sizeof(x));\n"
				"/*$TET$*/\n"
				"\t}\n\n"
				"\tvoid restore(restorer*r){\n"
				"/*$TET$" << m.name << "$$restore*/\n"
				"\t\t//::restore(r, &x, sizeof(x));\n"
				"/*$TET$*/\n"
				"\t}\n";
		}
			
		outf << "/*$TET$" << m.name << "$$data*/\n"
			"/*$TET$*/\n";

		outf << "};\n\n";
	}

	for(std::list<actor>::iterator it = alist.begin(); it!=alist.end();it++){
		actor& a = *it;

		outf << "#pragma templet "; print_actor(outf, a); outf << endl << endl;
		outf << "struct " << a.name << " : actor_interface{\n";

		outf << "\t" << a.name << "(engine_interface&){\n"
			"/*$TET$" << a.name << "$" << a.name << "*/\n"
			"/*$TET$*/\n"
			"\t}\n";

		if (!a.ports.empty()) outf << endl;

		for (std::list<port>::iterator it = a.ports.begin(); it != a.ports.end(); it++) {
			port& p = *it;
		
			if (p.type == port::CLIENT)
				outf << "\t" << p.message_name << " " << p.name << ";\n";
			else if (p.type == port::SERVER)
				outf << "\tvoid " << p.name << "(" << p.message_name << "&){}\n";
			else if (p.type == port::TASK) {
				outf << "\t" << p.message_name << " " << p.name << ";\n";
				outf << "\tvoid " << p.name << "_submit(){}\n";
			}
		}
		
		if (a.initially_active){
			outf << endl;
			outf << "\tvoid start(){\n"
				"/*$TET$" << a.name << "$start*/\n"
				"/*$TET$*/\n"
				"\t}\n";
		}

		for (std::list<port>::iterator it = a.ports.begin(); it != a.ports.end(); it++) {
			port& p = *it;

			outf << endl;
			outf << "\tvoid " << p.name << "_handler(" << p.message_name << "&" << (p.type==port::TASK?'t':'m') <<"){\n"
				"/*$TET$" << a.name << "$" << p.name << "*/\n"
				"/*$TET$*/\n"
				"\t}\n";
		}

		if (a.serilizable){
			outf << endl;
			outf << "\tvoid save(saver*s){\n"
				"/*$TET$" << a.name << "$$save*/\n"
				"\t\t//::save(s, &x, sizeof(x));\n"
				"/*$TET$*/\n"
				"\t}\n\n"
				"\tvoid restore(restorer*r){\n"
				"/*$TET$" << a.name << "$$restore*/\n"
				"\t\t//::restore(r, &x, sizeof(x));\n"
				"/*$TET$*/\n"
				"\t}\n";
		}

		outf << endl;
		outf << "/*$TET$" << a.name << "$$code&data*/\n"
			"/*$TET$*/\n";

		outf << "};\n\n";
	}

	outf << "/*$TET$code&data*/\n"
		"/*$TET$*/\n\n";
	
	outf << "int main(int argc, char *argv[])\n"
		"{\n"
		"\tengine_interface e(argc, argv);\n";

	outf << "/*$TET$footer*/\n"
			"/*$TET$*/\n";
	
	outf << "}\n";
}

void deploy(ofstream&outf, list<message>&mlist, list<actor>&alist)
{
	outf << "/*$TET$actor*/\n"
		"#include <templet.hpp>\n"
		"/*$TET$*/\n";

	outf << endl;

	outf << "using namespace TEMPLET;\n\n"
		"struct my_engine : engine{\n"
		"\tmy_engine(int argc, char *argv[]){\n"
		"\t\t::init(this, argc, argv);\n"
		"\t}\n"
		"\tbool run(){ return TEMPLET::run(this); }\n"
		"\tvoid map(){ TEMPLET::map(this); }\n"
		"};\n";

	outf << endl;

	for (std::list<message>::iterator it = mlist.begin(); it != mlist.end(); it++) {
		message& m = *it;

		outf << "#pragma templet "; print_message(outf, m); outf << endl << endl;
		outf << "struct " << m.name << " : message{\n";

		if (m.duplex)	outf << "\t" << m.name << "(actor*a, engine*e, int t) : _where(CLI), _cli(a), _client_id(t){\n";
		else outf << "\t" << m.name << "(actor*a, engine*e){\n";

		if (m.serilizable)	outf <<	"\t\t::init(this, a, e, "<<m.name<<"_save_adapter, "<<m.name<<"_restore_adapter);\n";
		else outf << "\t\t::init(this, a, e);\n";

		outf << "\t}\n";

		if (m.serilizable){
			outf << endl;

			outf << "\tstatic void "<<m.name<<"_save_adapter(message*m, saver*s){\n"
				"\t\t(("<< m.name <<"*)m)->save(s);\n"
				"\t}\n\n"
				"\tstatic void "<<m.name<<"_restore_adapter(message*m, restorer*r){\n"
				"\t\t(("<< m.name <<"*)m)->restore(r);\n"
				"\t}\n";
		}

		outf << endl;

		if (m.duplex){
			
			outf <<
				"\tvoid send(){\n"
				"\t\tif (_where == CLI){ TEMPLET::send(this, _srv, _server_id); _where = SRV; }\n"
				"\t\telse if (_where == SRV){ TEMPLET::send(this, _cli, _client_id); _where = CLI; }\n"
				"\t}\n";

		}
		else{
			outf <<
				"\tvoid send(){\n"
				"\t\tTEMPLET::send(this, _srv, _server_id);\n"
				"\t}\n";
		}

		if (m.serilizable){
			outf << endl;
			outf << "\tvoid save(saver*s){\n"
				"/*$TET$" << m.name << "$$save*/\n"
				"\t\t//TEMPLET::save(s, &x, sizeof(x));\n"
				"/*$TET$*/\n"
				"\t}\n\n"
				"\tvoid restore(restorer*r){\n"
				"/*$TET$" << m.name << "$$restore*/\n"
				"\t\t//TEMPLET::restore(r, &x, sizeof(x));\n"
				"/*$TET$*/\n"
				"\t}\n";
		}

		outf << endl;
		outf << "/*$TET$" << m.name << "$$data*/\n"
			"/*$TET$*/\n";

		if (m.duplex){
			outf << endl;
			outf <<
				"\tenum { CLI, SRV } _where;\n"
				"\tactor* _srv;\n"
				"\tactor* _cli;\n"
				"\tint _client_id;\n"
				"\tint _server_id;\n";
		}
		else {
			outf << endl;
			outf <<
				"\tactor* _srv;\n"
				"\tint _server_id;\n";
		}
		
		outf << "};\n\n";
	}

	for (std::list<actor>::iterator it = alist.begin(); it != alist.end(); it++) {
		actor& a = *it;

		outf << "#pragma templet "; print_actor(outf, a); outf << endl << endl;
		outf << "struct " << a.name << " : actor{\n";

		if (!a.ports.empty()){
			int tag_num = 0;

			outf << "\tenum tag{START";
			for (std::list<port>::iterator it1=a.ports.begin();it1!=a.ports.end();it1++){
				port& x = *it1;
				outf << ",TAG_" << x.name;
			}
			outf << "};\n\n";
		}

		bool have_client_ports = false;

		for (std::list<port>::iterator it = a.ports.begin(); it != a.ports.end(); it++) {
			port& x = *it;
			
			if (x.type == port::CLIENT || x.type == port::TASK) { have_client_ports = true; break; };
		}

		if (!have_client_ports)
			outf << "\t" << a.name << "(my_engine&e){\n";
		else{
			outf << "\t" << a.name << "(my_engine&e):";
			bool first = true;
			for(std::list<port>::iterator it1=a.ports.begin();it1!=a.ports.end();it1++){
				port& x = *it1;
				if (x.type == port::CLIENT){
					if (first)first = false; else outf << ",";
					outf << x.name << "(this, &e, TAG_" << x.name << ")";
				};
				if (x.type == port::TASK) {
					if (first)first = false; else outf << ",";
					outf << x.name << "(*(e._teng))";
				}
			}
			outf << "{\n";
		}

		if(a.serilizable) outf << "\t\tTEMPLET::init(this, &e, "<<a.name<<"_recv_adapter, "<<a.name<<"_save_adapter, "<<a.name<<"_restore_adapter);\n";
		else outf << "\t\tTEMPLET::init(this, &e, "<<a.name<<"_recv_adapter);\n";

		if (a.initially_active){
			outf << "\t\tTEMPLET::init(&_start, this, &e);\n"
				"\t\tTEMPLET::send(&_start, this, START);\n";
		}

		for (std::list<port>::iterator it1 = a.ports.begin(); it1 != a.ports.end(); it1++) {
			port& x = *it1;
			if (x.type == port::TASK) {
				outf << "\t\t" << x.name << ".set_on_ready([&]() { " << x.name <<  "_handler(" << x.name << "); resume(); });\n";
			}
		}

		outf <<
			"/*$TET$" << a.name << "$" << a.name << "*/\n"
			"/*$TET$*/\n"
			"\t}\n\n";

		if (a.serilizable){
			outf <<
				"\tstatic void "<<a.name<<"_save_adapter(actor*a, saver*s){\n"
				"\t\t((" << a.name << "*)a)->save(s);\n"
				"\t}\n\n"

				"\tstatic void "<<a.name<<"_restore_adapter(actor*a, restorer*r){\n"
				"\t\t((" << a.name << "*)a)->restore(r);\n"
				"\t}\n\n";
		}

		outf <<
			"\tbool access(message*m){ return TEMPLET::access(m, this); }\n"
			"\tbool access(message&m){ return TEMPLET::access(&m, this); }\n\n"
			"\tvoid at(int _at){ TEMPLET::at(this, _at); }\n"
			"\tvoid delay(double t){ TEMPLET::delay(this, t); }\n"
			"\tdouble time(){ return TEMPLET::time(this); }\n"
			"\tvoid stop(){ TEMPLET::stop(this); }\n";

		if (!a.ports.empty()) outf << endl;

		for (std::list<port>::iterator it = a.ports.begin(); it != a.ports.end(); it++) {
			port& p = *it;
		
			if (p.type == port::CLIENT)
				outf << "\t" << p.message_name << " " << p.name << ";\n";
			else if (p.type == port::SERVER)
				outf << "\tvoid " << p.name << "(" << p.message_name << "&m){m._server_id=TAG_"<<p.name<<"; m._srv=this;}\n";
			else if (p.type == port::TASK) {
				outf << "\t" << p.message_name << " " << p.name << ";\n";
				outf << "\t" << "void " << p.name << "_submit() { "<< p.name <<".submit(); suspend(); }" << ";\n";
			}
		}

		outf << endl;

		outf <<	"\tstatic void "<<a.name<<"_recv_adapter (actor*a, message*m, int tag){\n";
		outf << "\t\tswitch(tag){\n";

		for (std::list<port>::iterator it = a.ports.begin(); it != a.ports.end(); it++) {
			port& p = *it;
			if(p.type != port::TASK)
				outf << "\t\t\tcase TAG_" << p.name << ": ((" << a.name << "*)a)->" << p.name << "_handler(*((" << p.message_name << "*)m)); break;\n";
		}

		if (a.initially_active) outf << "\t\t\tcase START: (("<< a.name <<"*)a)->start(); break;\n";
		
		outf << "\t\t}\n";
		outf <<	"\t}\n";

		if (a.initially_active){
			outf << endl;
			outf << "\tvoid start(){\n"
				"/*$TET$" << a.name << "$start*/\n"
				"/*$TET$*/\n"
				"\t}\n";
		}

		for (std::list<port>::iterator it = a.ports.begin(); it != a.ports.end(); it++) {
			port& p = *it;

			outf << endl;
			outf << "\tvoid " << p.name << "_handler(" << p.message_name << "&"<< (p.type==port::TASK?'t':'m') <<"){\n"
				"/*$TET$" << a.name << "$" << p.name << "*/\n"
				"/*$TET$*/\n"
				"\t}\n";
		}

		if (a.serilizable){
			outf << endl;
			outf << "\tvoid save(saver*s){\n"
				"/*$TET$" << a.name << "$$save*/\n"
				"\t\t//TEMPLET::save(s, &x, sizeof(x));\n"
				"/*$TET$*/\n"
				"\t}\n\n"
				"\tvoid restore(restorer*r){\n"
				"/*$TET$" << a.name << "$$restore*/\n"
				"\t\t//TEMPLET::restore(r, &x, sizeof(x));\n"
				"/*$TET$*/\n"
				"\t}\n";
		}

		outf << endl;
		outf << "/*$TET$" << a.name << "$$code&data*/\n"
			"/*$TET$*/\n";

		if(a.initially_active) outf << "\tmessage _start;\n";

		outf << "};\n\n";
	}

	outf << "/*$TET$code&data*/\n"
		"/*$TET$*/\n\n";

	outf << "int main(int argc, char *argv[])\n"
		"{\n"
		"\tmy_engine e(argc, argv);\n";

	outf << "/*$TET$footer*/\n"
		"/*$TET$*/\n";

	outf << "}\n";
}

int main(int argc, char *argv[])
{
	if (argc<4){
		cout << "Templet skeleton generator. Copyright Sergei Vostokin, 2016" << endl << endl
			<< "Usage: gen (-deploy|-design) <input file with the templet markup> <skeleton output file>" << endl << endl
			<< "The Templet markup syntax:" << endl << endl
			<< "#pragma templet '~' id ['$'] ['=']" << endl
			<< "#pragma templet '*' id ['$'] \n\t [ '(' id ('!'|'?') id {',' id ('!'|'?') id)} ')' ] ['+']";

		return EXIT_FAILURE;
	}

	bool gen_deploy;
	string arg1(argv[1]);

	if (arg1 == "-deploy")
		gen_deploy = true;
	else if (arg1 == "-design")
		gen_deploy = false;
	else{
		cout << "ERROR: the first parameter '" << arg1 << "' is invalid, -d or -r expected" << endl;
		return EXIT_FAILURE;
	}

	
	string argv2(argv[2]), tt("templet");
	if (!openparse(argv2,tt)){
		cout << "ERROR: could not open file '" << argv[2] << "' for reading" << endl;
		return EXIT_FAILURE;
	}

	list<message> mlist;
	list<actor> alist;

	int line;
	string pragma;

	actor act;
	message msg;

	while (getpragma(pragma,line)){
		cout << "line:" << line << endl;
		cout << pragma << endl;

		lexinit(pragma);
		act.ports.clear();

		if (parse_message(line, msg))
			mlist.push_back(msg);
		else if (ungetlex() && parse_actor(line, act))
			alist.push_back(act);
		else
			error(line);
	}

	ofstream outf(argv[3]);

	if (!outf){
		cout << "ERROR: could not open file '" << argv[3] << "' for writing" << endl;
		return EXIT_FAILURE;
	}

	if(gen_deploy)
		deploy(outf, mlist, alist);
	else
		design(outf, mlist, alist);

	closeparse();

	return EXIT_SUCCESS;
}
