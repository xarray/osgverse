#pragma once

namespace ime{ namespace pinyin{

//候选词有三个类型：组合或联想、词库搜索、单个汉字
enum class CandidateType
{
	Combine,
	Associate,
	Query,
	Hanzi,
};

struct Candidate
{
	CandidateType	type;
	int				size;
	char			*pinyin;
	char			*cizu;
};

}}