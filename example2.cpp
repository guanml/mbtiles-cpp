#include <iostream>
#include <cstring>
#include <map>
#include <fstream>
#include "cppGzip/DecodeGzip.h"
#include "cppGzip/EncodeGzip.h"
#include <math.h>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include "VectorTile.h"
using namespace std;

//curl https://api.mapbox.com/v4/mapbox.mapbox-streets-v7/3/2/3.vector.pbf?access_token=<API_key> > map.bin

int ReadFileContents(const char *filename, int binaryMode, std::string &contentOut)
{
	contentOut = "";
	std::ios_base::openmode mode = (std::ios_base::openmode)0;
	if(binaryMode)
		mode ^= std::ios::binary;
	std::ifstream file(filename, mode);
	if(!file) return 0;

	file.seekg(0,std::ios::end);
	std::streampos length = file.tellg();
	file.seekg(0,std::ios::beg);

	std::vector<char> buffer(length);
	file.read(&buffer[0],length);

	contentOut.assign(&buffer[0], length);
	return 1;
}

///This class would need to be expanded to store features as needed.
class ExampleDataStore : public DecodeVectorTileResults
{
public:
	ExampleDataStore() : DecodeVectorTileResults()
	{
		cout << "Create custom data store..." << endl;
	}

	void Feature(int typeEnum, bool hasId, unsigned long long id, 
		const std::map<std::string, std::string> &tagMap,
		std::vector<Point2D> &pointsOut, 
		std::vector<std::vector<Point2D> > &linesOut,
		std::vector<Polygon2D> &polygonsOut)
	{
		//In real use, delete this function call and add your own functionality.
		DecodeVectorTileResults::Feature(typeEnum, hasId, id, 
			tagMap, pointsOut, linesOut, polygonsOut);
	}
};

int main(int argc, char *argv[])
{
	// Verify that the version of the library that we linked against is
	// compatible with the version of the headers we compiled against.
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	std::string finaIn = "map.vector.pbf";
	if (argc >= 2)
		finaIn = argv[1];
	std::string finaOut = "mapout.vector.pbf";
	if (argc >= 3)
		finaOut = argv[2];

	std::filebuf fb;
	std::filebuf* ret = fb.open(finaIn.c_str(), std::ios::in | std::ios::binary);
	if (ret == NULL)
	{
		cout << "Error opening input file" << endl;
		exit(0);
	}

	//Ungzip the data
	DecodeGzip dec(fb);
	string tileData;
	char tmp[1024];
	while(dec.in_avail())
	{
		streamsize bytes = dec.sgetn(tmp, 1024);
		tileData.append(tmp, bytes);
	}
	fb.close();

	//Decode vector data to stdout
	class ExampleDataStore results;
	class DecodeVectorTile vectorDec(3, 2, 3, results);
	vectorDec.DecodeTileData(tileData);

	//Reencode data to file
	std::stringstream binData;
	class EncodeVectorTile vectorEnc(3, 2, 3, binData);
	class DecodeVectorTile vectorDec2(3, 2, 3, vectorEnc);
	vectorDec2.DecodeTileData(tileData);
	std::string uncompData = binData.str();

	//Compress with gzip
	std::filebuf fb2;
	std::filebuf* ret2 = fb2.open(finaOut.c_str(), std::ios::out | std::ios::binary);
	if (ret2 == NULL)
		cout << "Error opening output file" << endl;
	EncodeGzip *enc = new EncodeGzip(fb2, Z_DEFAULT_COMPRESSION);
	enc->sputn(uncompData.c_str(), uncompData.length());
	delete enc; //Needed to complete the gzip file
	fb2.close();

	// Optional:  Delete all global objects allocated by libprotobuf.
	google::protobuf::ShutdownProtobufLibrary();

}

