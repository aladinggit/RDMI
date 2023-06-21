#ifndef HELPER_H
#define HELPER_H

#define LINK_BANDWIDTH 10
bool LINK_REG = true;

string read_file (string fname) {
	ifstream infile(fname);
	assert(infile.is_open());

	string code, l;
	while (getline(infile, l))
		code += l + "\n";
	return code;
}

#endif