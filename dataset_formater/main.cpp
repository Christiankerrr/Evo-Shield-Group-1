#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

int getCSVRowCount(fstream &);
int getCSVColumnCount(string &);
void getCSVRowTokens(string &, vector<string> &);
void printStringVector(vector<string> &);
void printStringVectorVector(vector<vector<string>> &);
bool isString(string &);
bool isIndexInVector(int, vector<int> &);
int getIndexFromVector(int, vector<int>&);
bool isStringInVector(string&, vector<string>&);

int main()
{
	fstream file;
	fstream outfile;

	file.open("file2.txt");

	if (!file.is_open())
	{
		cout << "Could not open file\n";
	}

	string str;

	const int line_count = getCSVRowCount(file);

	cout << "File's line count: " << getCSVRowCount(file) << endl << endl;


	vector<vector<string>> csv_data;

	//delimit strings, load into buffer
	while (!file.eof())
	{
		getline(file, str);
		
		vector<string> tokens;
		getCSVRowTokens(str, tokens);
	
		csv_data.push_back(tokens);
	}

	printStringVectorVector(csv_data); //optional for debugging, can be commented out to speed up

	//figure out which columns are strings
	vector<int> string_column_indecies;

	for (int i = 0; i < csv_data.size(); i++)
	{
		vector<string> temp = csv_data.at(i);

		for (int j = 0; j < temp.size(); j++)
		{
			if ( isString( temp.at(j) ) )
			{
				if (!isIndexInVector(j, string_column_indecies))
				{
					string_column_indecies.push_back(j);
				}
			}
		}
	}

	// we know what columns have strings, now find all possible strings for each column
	vector<vector<string>> possible_strings;
	for (int i = 0; i < string_column_indecies.size(); i++)
	{
		vector<string> temp;

		for (int j = 0; j < csv_data.size(); j++)
		{
			if (!isStringInVector(csv_data.at(j).at(string_column_indecies.at(i)), temp))
			{
				temp.push_back(csv_data.at(j).at(string_column_indecies.at(i)));
			}
		}

		possible_strings.push_back(temp);
	}

	printStringVectorVector(possible_strings); //optional for debugging, can be commented out to speed up

	//we have the info we need, now need to build csv back up
	cout << endl;

	vector<string> out_buffer;

	for (int i = 0; i < csv_data.size(); i++)
	{
		string temp = "";
		for (int j = 0; j < csv_data.at(i).size(); j++)
		{
			if (isIndexInVector(j, string_column_indecies))
			{
				int index = getIndexFromVector(j, string_column_indecies); //i am sorry for this

				for (int k = 0; k < possible_strings.at(index).size(); k++)
				{
					if (possible_strings.at(index).at(k).compare(csv_data.at(i).at(j)) == 0) //dont you just love weird logic
					{
						 temp = temp + "1.0,";
					}
					else
					{
						temp = temp + "0.0,";
					}
				}
			}
			else
			{
				temp = temp + csv_data.at(i).at(j) + ",";
			}
		}

		temp.erase(temp.end() - 1); //remove comma at end of line
		cout << temp << endl;
		
		out_buffer.push_back(temp);
	}

	ofstream{ "output.txt" }; //hack to create file

	outfile.open("output.txt");

	if (!outfile.is_open())
	{
		cout << "Could not open output file\n";
	}

	for (int i = 0; i < out_buffer.size(); i++)
	{
		outfile << out_buffer.at(i) << endl;
	}

	outfile.close();
	file.close();

	cout << endl << endl;
	return 0;
}

int getCSVRowCount(fstream & f)
{
	int count = 0;
	string buffer;

	while (!f.eof())
	{
		count++;
		getline(f, buffer);
	}

	f.seekg(0, f.beg);

	return count;
}

int getCSVColumnCount(string & s)
{
	int count = 0;
	int start = 0;
	int end = 0;

	while (end < s.size())
	{
		count++;

		end = s.find(',', start);

		start = end + 1;
	}

	return count;
}

void getCSVRowTokens(string& s, vector<string> & t)
{
	int start = 0;
	int end = 0;


	//debug
	//cout << "Running getCSVRowTokens on - " << s << endl;

	while (end < s.size())
	{
		end = s.find(',', start);
		t.push_back(s.substr(start, end - start));

		start = end + 1;
	}
}

void printStringVector(vector<string>& v)
{
	for (int i = 0; i < v.size(); i++)
	{
		cout << v.at(i) << ", ";
	}
	cout << endl;
}

void printStringVectorVector(vector<vector<string>>& vv)
{
	for (int i = 0; i < vv.size(); i++)
	{
		printStringVector(vv.at(i));
	}
}

bool isString(string & s)
{
	for (int i = 0; i < s.length(); i++)
	{
		//return false if a char in s is not between 46 and 57 and is not null-terminator
		if ( !(s[i] <= 57 && s[i] >= 46) && s[i] != 0)
		{
			return true;
		}
	}

	return false; 
}

bool isIndexInVector(int idx, vector<int>& indecies)
{
	for (int i = 0; i < indecies.size(); i++)
	{
		if (indecies.at(i) == idx) return true;
	}

	return false;
}

int getIndexFromVector(int idx, vector<int>& indecies)
{
	for (int i = 0; i < indecies.size(); i++)
	{
		if (indecies.at(i) == idx) return i;
	}

	return -1;
}

bool isStringInVector(string & s, vector<string>& sv)
{
	for (int i = 0; i < sv.size(); i++)
	{
		if (sv.at(i).compare(s) == 0) return true;
	}

	return false;
}
