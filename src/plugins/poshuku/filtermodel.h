#ifndef FILTERMODEL_H
#define FILTERMODEL_H
#include <plugininterface/tagsfiltermodel.h>

namespace LeechCraft
{
	namespace Plugins
	{
		namespace Poshuku
		{
			class FilterModel : public LeechCraft::Util::TagsFilterModel
			{
				Q_OBJECT
			public:
				FilterModel (QObject* = 0);
				virtual ~FilterModel ();
			protected:
				virtual QStringList GetTagsForIndex (int) const;
			};
		};
	};
};

#endif

