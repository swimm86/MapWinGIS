#include "stdafx.h"
#include "BingMapProvider.h"
#include "SecureHttpClient.h"

// ******************************************************
//    Initialize()
// ******************************************************
// Runs imagery metadata request: http://msdn.microsoft.com/en-us/library/ff701716.aspx
bool BingBaseProvider::Initialize()
{
	if (_urlFormat.GetLength() > 0) return true;

	if (m_globalSettings.bingApiKey.GetLength() == 0)
	{
		CallbackHelper::ErrorMsg("No Bing Maps API key was provided. See GlobalSettings.BingApiKey.");
		return false;
	}

	_initAttemptCount++;
	if (_initAttemptCount > 3)
	{
		CallbackHelper::ErrorMsg("Number of initialization attempts for Bing Maps provider was exceeded (3).");
		return false;
	}

	SecureHttpClient client;
	client.SetProxyAndAuthentication(_proxyUsername, _proxyPassword, _proxyDomain);

	//CAtlNavigateData navData;

	CString url;
	url.Format("http://dev.virtualearth.net/REST/v1/Imagery/Metadata/%s?key=%s&o=xml", _imagerySet, m_globalSettings.bingApiKey);

	bool result = false;

	if (!client.Navigate(url/*, &navData*/) || client.GetStatus() != 200)
	{
		CallbackHelper::ErrorMsg(Debug::Format("Failed to perform imagery metadata request. URL: ", url));
		return false;
	}

	return ParseUrlFormat(reinterpret_cast<void*>(&client));
}

// ******************************************************
//    ParseUrlFormat()
// ******************************************************
bool BingBaseProvider::ParseUrlFormat(void* secureHttpClient)
{
	SecureHttpClient* client = reinterpret_cast<SecureHttpClient*>(secureHttpClient);

	int bodyLen = client->GetBodyLength();
	if (bodyLen > 0)
	{
		char* body = new char[bodyLen + 1];
		memcpy(body, client->GetBody(), bodyLen);
		CString s = body;
		delete[] body;
		s = s.MakeLower();

		int pos = s.Find("<imageurl>");
		int pos2 = s.Find("</imageurl>");
		s = s.Mid(pos + 10, pos2 - pos - 10);
		s.Replace("&amp;", "&");

		if (s.GetLength() == 0)
			return false;

		_urlFormat = s;
		return true;
	}

	return false;
}

// ******************************************************
//    TileXYToQuadKey()
// ******************************************************
// Converts tile XY coordinates into a QuadKey at a specified level of detail.
// LevelOfDetail: Level of detail, from 1 (lowest detail) to 23 (highest detail).
CString BingBaseProvider::TileXYToQuadKey(int tileX, int tileY, int levelOfDetail)
{
	CString s;
	for (int i = levelOfDetail; i > 0; i--)
	{
		char digit = '0';
		int mask = 1 << (i - 1);
		if ((tileX & mask) != 0)
		{
			digit++;
		}
		if ((tileY & mask) != 0)
		{
			digit++;
			digit++;
		}

		s.AppendChar(digit);
	}
	return s;
}

// ******************************************************
//    MakeTileImageUrl()
// ******************************************************
CString BingBaseProvider::MakeTileImageUrl(CPoint &pos, int zoom)
{
	// http://ecn.{subdomain}.tiles.virtualearth.net/tiles/r{quadkey}.jpeg?g=3179&mkt={culture}&shading=hill
	CString key = TileXYToQuadKey(pos.x, pos.y, zoom);
	CString subDomain;
	subDomain.Format("t%d", GetServerNum(pos, 4));

	CString temp = _urlFormat;
	temp.Replace("{quadkey}", key);
	temp.Replace("{culture}", LanguageStr);
	temp.Replace("{subdomain}", subDomain);

	return temp;
}

// ******************************************************
//    GetCopyright()
// ******************************************************
CStringW BingBaseProvider::GetCopyright()
{
	if (_urlFormat.GetLength() == 0) {
		return "INVALID BING MAPS API KEY";
	}
	else {
		return _copyright;
	}
}