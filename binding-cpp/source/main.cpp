
#include <buffer.hpp>
#include <iostream>
#include <cassert>
#include <cstring>

using namespace BufferLib;


int main()
{

	Buffer<char> string = Buffer<char>({ 'R', 'A', 'V', 'I' , 0 });

	string.insertPseudo(2, 2);
	char* ptr = string.getPtrAt(2);
	ptr[0] = '5';
	ptr[1] = '&';
	string.log();
	std::cout << strlen(string) + 1 << std::endl;
	string.insertAt(strlen(string), '5');
 	
	for(char value: string)
		std::cout << value;
	std::cout << std::endl;

	Buffer<int> buffer = Buffer<int> ({ 1, 3, 5, 6 });

	buffer.push(7);
	buffer.push(8);
	buffer.push(9);

	buffer.push(100);
	buffer.push(200);
	buffer.push(300);

	for(int value : buffer)
	{
		std::cout << value << " ";	
	}

	buffer.clear();

	for(int i = 0; i < 10; i++)
		buffer.push(i);


	std::cout << "\nPrint Array\n";
	for(int value : buffer)
	{
		std::cout << value << " ";	
	}

	std::cout << "\nPOP\n";
	for(int i = 0; i < 4; i++)
		std::cout << buffer.pop() << " ";

	buffer.insertAt(0, 100);
	std::cout << "\nInsertion\n";
	for(int value: buffer)
		std::cout << value << " ";


	buffer.remove(100);
	std::cout << "\nRemoval\n";
	for(int value: buffer)
		std::cout << value << " ";

	std::cout << "\nFinding Index\n";
	size_t index = buffer.findIndexOf(3);
	std::cout << buffer[index] << std::endl;

	return 0;
}

