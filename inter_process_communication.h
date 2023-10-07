

// Message items
typedef struct msgobj {
char *pData;
exception Status;
struct l_obj *pBlock;
struct msgobj *pPrevious;
struct msgobj *pNext;
} msg;
// Mailbox structure
typedef struct {
msg *pHead;
msg *pTail;
int nDataSize;
int nMaxMessages;
int nMessages;
int nBlockedMsg;
} mailbox;

/* Communication */
mailbox* create_mailbox( uint nMessages, uint nDataSize );
exception remove_mailbox( mailbox* mBox );
exception send_wait( mailbox* mBox, void* pData );
exception receive_wait( mailbox* mBox, void* pData );
exception send_no_wait( mailbox* mBox, void* pData );
exception receive_no_wait( mailbox* mBox, void* pData );
