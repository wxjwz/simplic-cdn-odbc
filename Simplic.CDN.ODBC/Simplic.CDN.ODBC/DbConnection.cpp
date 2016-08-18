#include "stdafx.h"
#include "DbConnection.h"
#include "Statement.h"

#include "json/json.h"


DbConnection::DbConnection(Environment * env)
	: m_timeout(10)
{
	m_environment = env;
}

DbConnection::~DbConnection()
{
	LOCK(m_mutex);
	for (Statement* stmt : m_statements) delete stmt;
	m_statements.clear();
	m_environment->removeConnection(this);
}

Statement * DbConnection::createStatement()
{
	LOCK(m_mutex);
	Statement* stmt = new Statement(this);
	m_statements.insert(stmt);
	return stmt;
}

void DbConnection::removeStatement(Statement * stmt)
{
	LOCK(m_mutex);
	m_statements.erase(stmt);
}

void DbConnection::setTimeout(uint32_t timeout)
{
	m_timeout = timeout;
}

bool DbConnection::connect(std::string url, std::string user, std::string password)
{
	if (url.size() < 1) return false;
	if (url[url.size() - 1] != '/') url = url + "/"; // add trailing slash to url if it's not there yet
	url = url + "api/v1-0/";

	// TODO:
	// HTTP POST url + "auth/login" 
	// if(HTTP Status not OK) return false;

	// TODO: Remove this dummy!
	if (user == std::string("invalid")) return false;

	// the response will look like this dummy response
	std::string responseString = "{ \"token\": \"dummy.auth.token\" }";
	Json::Reader reader;
	Json::Value response;
	if (!reader.parse(responseString, response, false)) // parse responseString into response, skipping comments
	{
		return false;
	}

	
	m_url = url;
	m_authToken = "DummyAuthToken"; // we will get this from the auth api call
	return true;
}

Json::Value* DbConnection::executeCommand(const std::string & command, const Json::Value & parameters)
{
	// TODO:
	// use command in url: m_url + "odbc/" + command
	// use m_authToken in HTTP 'Authorization' header: "jwt " + m_authToken
	// use json-decoded parameters as content.

	// TODO:
	// decode the result as a json object
	// extract status information from HTTP status

	// dummy response:
	std::string responseString = "";

	if(command == "gettables") responseString = 
		"{"
		"    \"meta\" : {"
		"        \"rowcount\" : 1 ,"
		"        \"columns\" : ["
		"            { \"name\" : \"TABLE_CAT\" , \"type\" : 12 }," // type 12 is SQL_VARCHAR
		"            { \"name\" : \"TABLE_SCHEM\" , \"type\" : 12 },"
		"            { \"name\" : \"TABLE_NAME\" , \"type\" : 12 },"
		"            { \"name\" : \"TABLE_TYPE\" , \"type\" : 12 },"
		"            { \"name\" : \"TABLE_REMARKS\" , \"type\" : 12 }"
		"        ]"
		"    },"
		"    "
		"    \"rows\" : ["
		"        [\"catalog1\",\"schema1\",\"dummytable1\",\"TABLE\",\"This is just a dummy table that doesn't actually exist.\"]"
	//	"       ,[\"catalog1\",\"schema1\",\"dummytable2\",\"TABLE\",\"This is just a dummy table that doesn't actually exist.\"]"
		"    ]"
		"}";

	// SQLColumns() dummy result - see https://msdn.microsoft.com/de-de/library/ms711683(v=vs.85).aspx
	else if(command == "getcolumns") responseString = 
		"{"
		"    \"meta\" : {"
		"        \"rowcount\" : 2, "
		"        \"columns\" : ["
		"            { \"name\" : \"TABLE_CAT\"  , \"type\" : 12 }," // type 12 is SQL_VARCHAR
		"            { \"name\" : \"TABLE_SCHEM\", \"type\" : 12 },"
		"            { \"name\" : \"TABLE_NAME\" , \"type\" : 12, \"nullable\" : false },"
		"            { \"name\" : \"COLUMN_NAME\", \"type\" : 12, \"nullable\" : false },"
		"            { \"name\" : \"DATA_TYPE\"  , \"type\" : 5 , \"nullable\" : false }," // type 5 is SQL_SMALLINT
		"            { \"name\" : \"TYPE_NAME\", \"type\" : 12, \"nullable\" : false },"
		"            { \"name\" : \"COLUMN_SIZE\", \"type\" : 4 }," // type 4 is SQL_INTEGER
		"            { \"name\" : \"BUFFER_LENGTH\", \"type\" : 4 }," 
		"            { \"name\" : \"DECIMAL_DIGITS\", \"type\" : 5 },"
		"            { \"name\" : \"NUM_PREC_RADIX\", \"type\" : 5 },"
		"            { \"name\" : \"NULLABLE\", \"type\" : 5, \"nullable\" : false  },"
		"            { \"name\" : \"REMARKS\", \"type\" : 12 },"
		"            { \"name\" : \"COLUMN_DEF\", \"type\" : 12 },"
		"            { \"name\" : \"SQL_DATA_TYPE\", \"type\" : 5, \"nullable\" : false },"
		"            { \"name\" : \"SQL_DATETIME_SUB \", \"type\" : 5 },"
		"            { \"name\" : \"CHAR_OCTET_LENGTH  \", \"type\" : 4 },"
		"            { \"name\" : \"ORDINAL_POSITION  \", \"type\" : 4, \"nullable\" : false },"
		"            { \"name\" : \"IS_NULLABLE\", \"type\" : 12 }"
		"        ]"
		"    },"
		"    "
		"    \"rows\" : ["   
		//        TABLE_CAT     TABLE_SCHEM  TABLE_NAME       COLUMN_NAME  DATA_TYPE  TYPE_NAME    COLUMN_SIZE  BUFFER_LENGTH  DECIMAL_DIGITS  NUM_PREC_RADIX  NULLABLE  REMARKS  COLUMN_DEF     SQL_DATA_TYPE  SQL_DATETIME_SUB  CHAR_OCTET_LENGTH  ORDINAL_POSITION  IS_NULLABLE
		"        [\"catalog1\", \"schema1\", \"dummytable1\", \"id\"     , 4        , \"INTEGER\", 10         , 4            , null          , null          , 0       , \"\"   , \"0\"        , 4            , null            , null             , 1               , \"NO\" ],"
		"        [\"catalog1\", \"schema1\", \"dummytable2\", \"text\"   , 12       , \"VARCHAR\", 255        , 256          , null          , null          , 0       , \"\"   , \"'default'\", 4            , null            , 255              , 2               , \"NO\" ]"
		"    ]"
		"}";


	Json::Reader reader;

	if (!reader.parse(responseString, m_apiResult, false)) // parse responseString into response, skipping comments
	{
		m_apiResult = Json::Value(); // clear result object if parsing failed
	}

	return &m_apiResult;
}

bool DbConnection::fetch(std::vector<Json::Value*> & result, uint32_t fromRow, uint32_t numRows)
{
	result.clear();
	Json::Value& rows = m_apiResult["rows"];
	if (rows.isNull()) return false;

	uint32_t rowsAvailable = rows.size();

	for (uint32_t i = fromRow; i < fromRow + numRows && i < rowsAvailable; ++i)
	{
		result.push_back(&rows[i]);
	}

	return true;
}
