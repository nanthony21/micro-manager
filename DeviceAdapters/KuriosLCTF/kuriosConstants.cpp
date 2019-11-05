#include <map>

struct SR {typedef enum Type {VIS=1, NIR=2, IR=4};}; //spectral range
struct BW {typedef enum Type {BLACK=1, WIDE=2, MEDIUM=4, NARROW=8, SUPERNARROW=16};};


static std::map<SR::Type,std::string> create_SpectralRangeMap() {
	std::map<SR::Type, std::string> m;
	m[SR::VIS]="Visible";
	m[SR::NIR]="Near IR";
	m[SR::IR]="Infrared";
	return m;
};
static const std::map<SR::Type, std::string> SRNames = create_SpectralRangeMap();

static std::map<BW::Type,std::string> create_BandwidthMap() {
	std::map<BW::Type, std::string> m;
	m[BW::BLACK]= "Black";
	m[BW::WIDE]="Wide";
	m[BW::MEDIUM]="Medium";
	m[BW::NARROW]="Narrow";
	m[BW::SUPERNARROW]="Super Narrow";
	return m;
};
static const std::map<BW::Type, std::string> BWNames = create_BandwidthMap();

const char* g_LCTFName = "Kurios LCTF";