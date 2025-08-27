
#include "GuildRpgAction.h"
#include "PlayerbotAI.h"

bool GuildStatusUpdateAction::Execute(Event event)
{
    //Get Bot guild type
    //Based on guild type and probabilities from config determine bot objective. 
    //set bot status to that objective. 
}

bool ExecuteGuildTaskAction::Execute(Event event)
{
     = AI_VALUE(GuildTask, "guild task");
    if (!task.IsValid())
        return false;

    switch (task.type)
    {
        case ACTIVITY_PVP:
            return ExecutePvpTask(task); // <- We are here
 
    }
}
