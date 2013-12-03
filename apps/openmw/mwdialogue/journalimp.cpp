
#include "journalimp.hpp"

#include <iterator>

#include <components/esm/esmwriter.hpp>
#include <components/esm/esmreader.hpp>
#include <components/esm/queststate.hpp>
#include <components/esm/journalentry.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwgui/messagebox.hpp"

namespace MWDialogue
{
    Quest& Journal::getQuest (const std::string& id)
    {
        TQuestContainer::iterator iter = mQuests.find (id);

        if (iter==mQuests.end())
        {
            std::pair<TQuestContainer::iterator, bool> result =
                mQuests.insert (std::make_pair (id, Quest (id)));

            iter = result.first;
        }

        return iter->second;
    }

    Topic& Journal::getTopic (const std::string& id)
    {
        TTopicContainer::iterator iter = mTopics.find (id);

        if (iter==mTopics.end())
        {
            std::pair<TTopicContainer::iterator, bool> result
                = mTopics.insert (std::make_pair (id, Topic (id)));

            iter = result.first;
        }

        return iter->second;
    }


    Journal::Journal()
    {}

    void Journal::clear()
    {
        mJournal.clear();
        mQuests.clear();
        mTopics.clear();
    }

    void Journal::addEntry (const std::string& id, int index)
    {
        // bail out of we already have heard this...
        std::string infoId = JournalEntry::idFromIndex (id, index);
        for (TEntryIter i = mJournal.begin (); i != mJournal.end (); ++i)
            if (i->mTopic == id && i->mInfoId == infoId)
                return;

        StampedJournalEntry entry = StampedJournalEntry::makeFromQuest (id, index);

        mJournal.push_back (entry);

        Quest& quest = getQuest (id);

        quest.addEntry (entry); // we are doing slicing on purpose here

        std::vector<std::string> empty;
        std::string notification = "#{sJournalEntry}";
        MWBase::Environment::get().getWindowManager()->messageBox (notification, empty);
    }

    void Journal::setJournalIndex (const std::string& id, int index)
    {
        Quest& quest = getQuest (id);

        quest.setIndex (index);
    }

    void Journal::addTopic (const std::string& topicId, const std::string& infoId)
    {
        Topic& topic = getTopic (topicId);

        topic.addEntry (JournalEntry (topicId, infoId));
    }

    int Journal::getJournalIndex (const std::string& id) const
    {
        TQuestContainer::const_iterator iter = mQuests.find (id);

        if (iter==mQuests.end())
            return 0;

        return iter->second.getIndex();
    }

    Journal::TEntryIter Journal::begin() const
    {
        return mJournal.begin();
    }

    Journal::TEntryIter Journal::end() const
    {
        return mJournal.end();
    }

    Journal::TQuestIter Journal::questBegin() const
    {
        return mQuests.begin();
    }

    Journal::TQuestIter Journal::questEnd() const
    {
        return mQuests.end();
    }

    Journal::TTopicIter Journal::topicBegin() const
    {
        return mTopics.begin();
    }

    Journal::TTopicIter Journal::topicEnd() const
    {
        return mTopics.end();
    }

    int Journal::countSavedGameRecords() const
    {
        int count = static_cast<int> (mQuests.size());

        for (TQuestIter iter (mQuests.begin()); iter!=mQuests.end(); ++iter)
            count += std::distance (iter->second.begin(), iter->second.end());

        count += std::distance (mJournal.begin(), mJournal.end());

        for (TTopicIter iter (mTopics.begin()); iter!=mTopics.end(); ++iter)
            count += std::distance (iter->second.begin(), iter->second.end());

        return count;
    }

    void Journal::write (ESM::ESMWriter& writer) const
    {
        for (TQuestIter iter (mQuests.begin()); iter!=mQuests.end(); ++iter)
        {
            const Quest& quest = iter->second;

            ESM::QuestState state;
            quest.write (state);
            writer.startRecord (ESM::REC_QUES);
            state.save (writer);
            writer.endRecord (ESM::REC_QUES);

            for (Topic::TEntryIter iter (quest.begin()); iter!=quest.end(); ++iter)
            {
                ESM::JournalEntry entry;
                entry.mType = ESM::JournalEntry::Type_Quest;
                entry.mTopic = quest.getTopic();
                iter->write (entry);
                writer.startRecord (ESM::REC_JOUR);
                entry.save (writer);
                writer.endRecord (ESM::REC_JOUR);
            }
        }

        for (TEntryIter iter (mJournal.begin()); iter!=mJournal.end(); ++iter)
        {
            ESM::JournalEntry entry;
            entry.mType = ESM::JournalEntry::Type_Journal;
            iter->write (entry);
            writer.startRecord (ESM::REC_JOUR);
            entry.save (writer);
            writer.endRecord (ESM::REC_JOUR);
        }

        for (TTopicIter iter (mTopics.begin()); iter!=mTopics.end(); ++iter)
        {
            const Topic& topic = iter->second;

            for (Topic::TEntryIter iter (topic.begin()); iter!=topic.end(); ++iter)
            {
                ESM::JournalEntry entry;
                entry.mType = ESM::JournalEntry::Type_Topic;
                entry.mTopic = topic.getTopic();
                iter->write (entry);
                writer.startRecord (ESM::REC_JOUR);
                entry.save (writer);
                writer.endRecord (ESM::REC_JOUR);
            }
        }
    }

    void Journal::readRecord (ESM::ESMReader& reader, int32_t type)
    {
        if (type==ESM::REC_JOUR)
        {
            ESM::JournalEntry record;
            record.load (reader);

            switch (record.mType)
            {
                case ESM::JournalEntry::Type_Quest:

                    getQuest (record.mTopic).insertEntry (record);
                    break;

                case ESM::JournalEntry::Type_Journal:

                    mJournal.push_back (record);
                    break;

                case ESM::JournalEntry::Type_Topic:

                    getTopic (record.mTopic).insertEntry (record);
                    break;
            }
        }
        else if (type==ESM::REC_QUES)
        {
            ESM::QuestState record;
            record.load (reader);

            mQuests.insert (std::make_pair (record.mTopic, record));
        }
    }
}
