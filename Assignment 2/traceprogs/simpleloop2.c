#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct group {
	char *name;
	struct group *next;
};

typedef struct group Group;

int add_group(Group **group_list, const char *group_name);
void list_groups(Group *group_list);
Group *find_group(Group *group_list, const char *group_name);

/*
* Helper function to allocate memory
* the function will exit the program if there is no memory available
*/
static void *allocate_memory(size_t numBytes) {
    // allocate memory the size of given argument numBytes
    void *memoryVar = malloc(numBytes);
    // if memory cannot be allocated, exit the program
    if (memoryVar == NULL) {
        exit(-1);
    }
    // else return the allocated memory 
    return memoryVar;
}


/*
* Helper function to create new Group
* Since add_group only specifies group name, we can ignore the rest of the items in the group struct for now
*/
static Group *create_group(char *groupName) {
    // allocate memory for the new group node
    Group *newGroup = (Group *) allocate_memory(sizeof(Group));
    // store the length of the group name in nameLength
    size_t nameLength = strlen(groupName);
    // allocate memory for the group's name
    newGroup->name = (char *) allocate_memory(nameLength + 1);
    // copy the group name to newNode->name
    strncpy(newGroup->name, groupName, nameLength);
    // add the null terminator
    newGroup->name[nameLength + 1] = '\0';
    return newGroup;
}


/* Add a group with name group_name to the group_list referred to by 
* group_list_ptr. The groups are ordered by the time that the group was 
* added to the list with new groups added to the end of the list.
*
* Returns 0 on success and -1 if a group with this name already exists.
*
* (I.e, allocate and initialize a Group struct, and insert it
* into the group_list. Note that the head of the group list might change
* which is why the first argument is a double pointer.) 
*/
int add_group(Group **group_list_ptr, const char *group_name) {
    // Initialize a variable to use for return value
    int groupAdded = 0; // Set it to success status initially and change it to -1 if a group in group list already has the same name as group_name
    // Initialize Group pointer to traverse through the group list
    Group *currentNode = *group_list_ptr;
    // Check if group list is empty
    if (currentNode == NULL) {
	// First item is empty, so create a group and insert it as the first group in group list 
	*group_list_ptr = create_group((char *) group_name); // call helper function to create new group pointer and make it first item in group list
    } else {
	// while there is a next node and no matching node so far (i.e. group with group_name doesn't exist)
	while (currentNode->next != NULL && groupAdded != -1) {
	    // if current node doesnt have the same name as the new node we want to create, move to next node
	    if (strcmp(currentNode->name, (char *) group_name) != 0) {
		currentNode = currentNode->next;
	    } else {
		// matching node found (i.e. cannot add new node)
		groupAdded = -1;
	    }
	}
	// There are no more next nodes (i.e. we've reached the last item), so check if the last node has the same name as new node
	if (groupAdded != -1 && strcmp(currentNode->name, (char *) group_name) != 0)  {
	    currentNode->next = create_group((char *) group_name);
	} else {
	    // last node has the same name as the one we're trying to add (i.e. cannot add new node)
	    groupAdded = -1;
	}
    }
    return groupAdded;
}

/* Print to standard output the names of all groups in group_list, one name
*  per line. Output is in the same order as group_list.
*/
void list_groups(Group *group_list) {
    // point currentNode to the group list to traverse through the group items
    Group *currentNode = group_list;
    // if list is not empty, print the list items (groups)
    if (currentNode != NULL) {
	// while there is a group at current node, print the group name
	while (currentNode != NULL) {
            printf("Name of group: %s \n", currentNode->name);
  	    // move down the list (set node to next node)
            currentNode = currentNode->next;
	}
    } else{
	// empty list
	printf("There are no groups in the list\n");
    }
}

int main() {
    Group *groupOne = create_group("Group 1");
    Group **group_head = &groupOne;
    int counter;
    for (counter = 2; counter < 5000; counter++) {
	char group_label[5] = "Group";
        add_group(group_head, group_label);
    }
    list_groups(groupOne);
    return 0;
}
