#include "config/configfile.h"
#include "log/log.h"

#include <fstream>
#include <algorithm>
#include <iomanip>

using namespace kmicki::log;

namespace kmicki::config
{
    namespace cf {
        static const std::string cLogPrefix = "ConfigFile:";
        #include "log/locallog.h"
    }

    ConfigFile::ConfigFile(std::string const& _filePath)
    {
        filePath = _filePath;
        
        { cf::LogF(LogLevelTrace) << " Initialize with filepath: " << _filePath; }
    }

    const char ConfigFile::cSeparator = ':';
    const char cCommentPrefix = '#';

    bool ConfigFile::LoadConfig(std::vector<std::unique_ptr<ConfigItemBase>> & configuration) const
    {

        { cf::LogF(LogLevelTrace) << ":LoadConfig: Loading configuration from file: " << filePath; }
        std::ifstream file(filePath);
        if(file.fail())
        {
            cf::Log(":LoadConfig: File open failed.",LogLevelDebug);
            return false;
        }

        int line = 0;

        std::string precedingComment = "";

        while(!file.eof())
        {
            ++line;
            std::string str;
            std::getline(file,str);

            auto c = str.begin();
            while(std::isspace(*c))
                c = str.erase(c);

            // detect preceding comment
            if(*(str.begin()) == cCommentPrefix)
            {
                str.erase(str.begin());

                precedingComment.append(str);
                precedingComment.push_back('\n');

                { cf::LogF(LogLevelTrace) << ":LoadConfig: Line " << line << " - comment: " << str; }
                continue;
            }

            std::string precedingCommentAll = "";
            precedingCommentAll.swap(precedingComment);
            std::string inlineComment = "";

            // remove white space and comment
            c = str.begin();
            while(c != str.end())
                if(std::isspace(*c))
                    c = str.erase(c);
                else if(*c == cCommentPrefix)
                {
                    auto ch = c+1;
                    while(std::isspace(*ch))
                        ++ch;
                    inlineComment = std::string(ch,str.end());
                    c = str.erase(c,str.end());
                }
                else
                    ++c;

            if(str.empty())
            {
                { cf::LogF(LogLevelTrace) << ":LoadConfig: Line " << line << " - empty or comment"; }
                continue;
            }

            auto pos = str.find(cSeparator);
            if(pos == std::string::npos)
            {
                { cf::LogF(LogLevelDebug) << ":LoadConfig: Line " << line << " - invalid, no separator found (" << cSeparator << ")"; }
                continue;
            }

            if(pos == 0)
            {
                { cf::LogF(LogLevelDebug) << ":LoadConfig: Line " << line << " - invalid, no name found"; }
                continue;                
            }

            auto name = str.substr(0,pos);
            auto item = std::find_if(configuration.begin(),configuration.end(),
                            [&](std::unique_ptr<ConfigItemBase> & x)
                            { 
                                return x->Name == name;
                            }
                        );

            std::string value("");
            if(pos < str.length()-1)
                value = str.substr(pos+1);

            if(item == configuration.end())
            {
                { cf::LogF(LogLevelTrace) << ":LoadConfig: Line " << line << " - new item, Name=" << name << " Value=" << value; }
                configuration.emplace_back(new ConfigItem<std::string>(name,value,ConfigComment{ precedingCommentAll, inlineComment }));
                continue;
            }

            std::string msg;
            if((*item)->Update(value,msg))
            {
                { cf::LogF(LogLevelTrace) << ":LoadConfig: Line " << line << " - " << msg; }
            }
            else
                { cf::LogF(LogLevelDebug) << ":LoadConfig: Line " << line << " - " << msg; }
            if(!precedingCommentAll.empty())
                (*item)->Comment.PrecedingComment = precedingCommentAll;
            if(!inlineComment.empty())
                (*item)->Comment.InlineComment = inlineComment;
        }

        cf::Log(":LoadConfig: Configuration loaded.",LogLevelDebug);
        return true;
    }

    static bool InsertComment(std::ostream & stream, std::string const& comment, bool inlineComment = false)
    {
        bool add = !comment.empty();
        if(add)
        {
            if(inlineComment)
                stream << ' '; 
            stream << cCommentPrefix;
            if(!std::isspace(*(comment.cbegin())))
                stream << ' ';
            stream << comment;
        }
        stream << '\n';
        return add;
    }

    bool ConfigFile::SaveConfig(std::vector<std::unique_ptr<ConfigItemBase>> const& configuration,bool pretty) const
    {
        { cf::LogF(LogLevelTrace) << ":SaveConfig: Saving configuration to file: " << filePath; }
        std::ofstream file(filePath,std::ios_base::trunc);
        if(file.fail())
        {
            cf::Log(":SaveConfig: File open failed.",LogLevelDebug);
            return false;
        }
        
        int max = 0;

        if(pretty)
        {
            // Find longest name
            for(auto& item : configuration)
                if(item->Name.length() > max)
                    max = item->Name.length();
            { cf::LogF(LogLevelTrace) << ":SaveConfig: Pretty layout enabled. Found maximum name length: " << max; }
        }

        int line = 0;

        file << "# SteamDeckGyroDSU Configuration file\n\n";

        for(auto& item : configuration)
        {
            std::stringstream cmt(item->Comment.PrecedingComment);
            std::string cmtLine;
            while(std::getline(cmt,cmtLine))
                InsertComment(file,cmtLine);

            std::string value = item->ValToString();
            file << std::setw(max) << std::left << item->Name << ' ' << cSeparator << ' ' << value;
            InsertComment(file,item->Comment.InlineComment,true);
            file << "\n";
            { cf::LogF(LogLevelTrace) << ":SaveConfig: Line " << ++line << " - saved item, Name=" << item->Name << " Value=" << value; }
        }

        cf::Log(":SaveConfig: Configuration saved.",LogLevelDebug);

        return true;
    }
}