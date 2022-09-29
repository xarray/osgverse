#include <math.h>
#include "PinyinDivider.h"
#include "PinyinBase.h"
#include "StringFunction.h"

using namespace ime::pinyin;
#define PINYIN_MAX_SIZE						6

void PinyinDivider::preDivide(const std::string &input, std::vector<int> &points)
{
	unsigned int nStep = 0;
	//以下循环初步分割保存在points(即不考虑特殊的n和g点)
	//初步分割原则：从0位置开始以最长的拼音为单位分割，并以是否是向后匹配的点作为依据，如maning，分割为ma'ning
	//如果该位置有用户分隔符，就不插入
	for(size_t nPos = 0; nPos != input.size(); nPos += nStep)
	{
		nStep = 0;
		unsigned int nOriginalPoint = 0;
		unsigned int nSizeLimit = input.size() - nPos >= PINYIN_MAX_SIZE ? PINYIN_MAX_SIZE : input.size() - nPos;
		for(unsigned int nLen = nSizeLimit; nLen != 0; --nLen)
		{
			std::string sSubString = input.substr(nPos, nLen);
			bool bOneWay = PinyinBase::isOneway(sSubString);
			bool bCorrect = PinyinBase::isValid(sSubString);
			if(bOneWay || bCorrect)
			{
				nOriginalPoint = nPos + nLen - 1;
				//后一个字符是不是分隔符
				char nextChar = nOriginalPoint + 1 == input.size() ? 0 : input[nOriginalPoint + 1];
				if(nextChar == '\'')
				{
					nStep = nLen + 1;
				}
				else 
				{
					//到了末端不插入
					if(nPos + nLen != input.size())
					{
						std::string s = sSubString.substr(0, sSubString.size() - 1);
						std::string ss = sSubString.substr(sSubString.size() - 1, 1) + nextChar;
						bool bWhole = PinyinBase::isCompleted(sSubString);
						if(m_setMatchBackOnly.find(nOriginalPoint) != m_setMatchBackOnly.end() || (!bWhole && PinyinBase::isCompleted(s) && PinyinBase::isYunmuIndependent(ss)))
						{
							points.push_back(nOriginalPoint - 1);
							nStep = nLen - 1;
						}
						else
						{
							points.push_back(nOriginalPoint);
							nStep = nLen;
						}
					}
					else
					{
						nStep = nLen;
					}
				}
				break;
			}
			else if(nLen == 1)		//i，u，v
			{
				nOriginalPoint = nPos;
				//后一个字符是不是分隔符
				char nextChar = nOriginalPoint + 1 == input.size() ? 0 : input[nOriginalPoint + 1];
				if(nextChar == '\'')
				{
					nStep = 2;
				}
				else 
				{
					//到了末端不插入
					if(nPos + nLen != input.size())
						points.push_back(nOriginalPoint);
					nStep = 1;
				}
				break;
			}
		}
	}
}

std::string PinyinDivider::divide(const std::string &input)
{
	if(input.empty())
		return "";

	//获取特殊点
	calcSpecialN_G(input);
	//初步分割
	std::vector<int> points;
	preDivide(input, points);
	std::string ret = StringFunction::insert(input, "'", std::set<int>(points.begin(), points.end()));
	if (ret.back() == '\'')
		ret.pop_back();

	return ret;
}

void PinyinDivider::calcSpecialN_G(const std::string &sInput)
{
	m_setMatchBackOnly.clear();
	m_vtMatchBoth.clear();
	for(int i = 0; i != sInput.size(); ++i)
	{
		char ch = sInput[i];
		if(ch == 'n')
		{
			if(i != 0 && i + 1 != sInput.size())
			{
				char prevChar = sInput[i - 1];
				char nextChar = sInput[i + 1];
				if(prevChar != '\'' && nextChar != '\'' && isVowelChar(nextChar)) //需要后一个字符是元音，且前后都不是分隔符
				{
					MatchAttr attr = getSpecialPointMatchAttr(sInput, i);
					switch(attr)
					{
					//初步分割已经是向前匹配，所有不用处理
					case MA_Match_Ahead_Only:											break;
					case MA_Match_Back_Only:	m_setMatchBackOnly.insert(i);			break;
					case MA_Match_Pre_Back:		m_vtMatchBoth.push_back({ i - 1, i });	break;
					default:															break;
					}
				}
			}
		}
		else if(ch == 'g')
		{
			if(i != 0 && i + 1 != sInput.size())
			{
				char prevChar = sInput[i - 1];
				char nextChar = sInput[i + 1];
				//后一个字符为不是'i'的元音字符且前一个字符是'n'且前后的字符都不是分隔符
				if((prevChar == 'n' && prevChar != '\'') && (nextChar != 'i' && nextChar != '\'' && isVowelChar(nextChar)))
				{
					MatchAttr attr = getSpecialPointMatchAttr(sInput, i);
					switch(attr)
					{
					//初步分割已经是向前匹配，所以不用处理
					case MA_Match_Ahead_Only:											break;
					case MA_Match_Back_Only:	m_setMatchBackOnly.insert(i);			break;
					case MA_Match_Pre_Back:		m_vtMatchBoth.push_back({ i - 1, i });	break;
					default:															break;
					}
				}
			}
		}
	}
}

bool PinyinDivider::isVowelChar(char ch)
{
	return ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u' || ch == 'v';
}

PinyinDivider::MatchAttr PinyinDivider::getSpecialPointMatchAttr(const std::string &sInput, int nN_PPosition)
{
	int nFrontBegin = nN_PPosition >= PINYIN_MAX_SIZE - 1 ? nN_PPosition - (PINYIN_MAX_SIZE - 1) : 0;
	int nFrontSizeLimit = nN_PPosition >= PINYIN_MAX_SIZE - 1 ? PINYIN_MAX_SIZE - 1 : nN_PPosition;
	for(int length = nFrontSizeLimit; length != 0; --length, ++nFrontBegin)
	{
		std::string sFrontSubStr = sInput.substr(nFrontBegin, length + 1);
		//可前匹配
		if(PinyinBase::isValid(sFrontSubStr))
		{
			//只可前匹配
			std::string sFront = sInput.substr(nFrontBegin, length);
			if(!PinyinBase::isValid(sFront) && !PinyinBase::isOneway(sFront))
			{
				return MA_Match_Ahead_Only;
			}
			else
			{
				int nBackBegin = nN_PPosition;
				int nBackSizeLimit = sInput.size() - nN_PPosition > PINYIN_MAX_SIZE - 1 ? PINYIN_MAX_SIZE - 1 : sInput.size() - nN_PPosition;
				for(int len = nBackSizeLimit; len != 1; --len)
				{
					std::string sBackSubStr = sInput.substr(nN_PPosition, len);
					if(PinyinBase::isValid(sBackSubStr) || PinyinBase::isOneway(sBackSubStr))
					{
						//只要不是i，u，v都可能是isValid
						if(!PinyinBase::isValid(sInput.substr(nN_PPosition + 1, len - 1)))
						{
							return MA_Match_Back_Only;
						}
						//注意，下面这句话才是对的，但是为了简便，return MA_Match_Back_Only，比如xiangong，g是可前后的特殊点
						//现在把它认为是只可向后匹配点，不然还要处理返回xian'gong和xiang'o'n'g，先简便的使他返回xian'gong
						else
						{
							//	return MA_Match_Pre_Back;
							return MA_Match_Back_Only;
						}
					}
				}
				return MA_Match_Ahead_Only;
			}
		}
	}
	return MA_Match_Ahead_Only;
}
