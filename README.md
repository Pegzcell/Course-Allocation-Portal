# Course-Allocation-Portal
An unconventional course registration system for a college, where a student can take trial classes of a course and can withdraw and opt for a different course if he/she/they do not like the course. Different labs in the college have been asked to provide students who can act as course Teaching Asiistent cum Mentors temporarily and take the trial tutorials.

## ENTITY STATE SUMMARY
- Student
	- Not registered yet
	- Waiting for a seat at the course he filled as his first preference (course 1)
	- Attending tutorial for course 1
	- Waiting for a seat at the course he filled as his second preference (course 2)
	- Attending tutorial for course 2
	- Waiting for a seat at the course he filled as his second preference (course 3)
	- Attending tutorial for course 3
	- Selected a course permanently
	- Did not like all 3 courses and has exited the simulation
- Course
	- Is currently waiting for a TA to become available
	- Has an allotted TA, waiting for slots to be filled
	- Has an allotted TA who is conducting tutorials
	- Has been withdrawn
- Lab
	- At Least one TA mentor who is a part of the lab has not completed his quota
	- All people in the lab have exhausted the limit on the number of times someone can accept a TAship and so, the lab will no longer send out any TAs

## INPUT FORMAT
	<num_students> <num_labs> <num_courses>
	<name of course 1> <interest quotient for course 1> <maximum number of slots which can be allocated by a TA> <number of labs
	from which course accepts TA> [list of lab IDs]
	.
	.
	.
	<name of the last course> <interest quotient for the last course> <maximum number of slots which can be allocated by a TA>
	<number of labs from which course accepts TA> [list of lab IDs]
	<calibre quotient of first student> <preference 1> <preference 2> <preference 3> <time after which he fills course preferences>
	.
	.
	<calibre quotient of last student> <preference 1> <preference 2> <preference 3> <time after which he fills course preferences>
	<name of first lab> <number of students/TA mentors in first lab> <number of times a member of the lab can TA in a course>
	.
	.
	.
	<name of last lab> <number of students.TA Mentors in the last lab> <number of times a member of the lab can TA in a course>

## REPORT

- the simulation is done by making threads for each `course` and `student`, and dynamic data arrays storing details of all `courses`, `students` and `labs`.
- the program terminates when all students have either selected a course permanently, or could not get any of their preferred courses, i.e. all student threads have returned to the main thread. This can be changed by un-commenting the `pthread_join()` statements for the course threads, so as to terminate the program only when all threads have returned, i.e when the courses are withdrawn due to lack of TA mentors.
- Every course thread can choose the TA parallely, only locking the lab it is currently searching in. The TA with the *least tutoring instances* from the lab is selected by calling the `available` function. The course thread initiates a `semaphore` with value equal to the slots allocated by the TA. The thread waits for a small `SLOTS_FILLING` time.
- The student threads after registering, wait on the semaphore corresponding to their current preferred course. 
- If selected, the execution of the student thread is temporarily suspended using a conditional variable that signals from the course thread, once the tutorial is completed after a period of `TUT_TIME`.
- Then, each student thread that completed the tutorial, calls the `decide` function to decide on selecting the said course permanently or moving to their next preference.
- the course thread returns once all TAs from labs that the course accepts have `EXHAUSTED`, i.e. have tutored the maximum number of times allowed in their respective labs.
- in case a course preferred by a student is `dropped`, the preference of the student waiting on it shifts to the next.
- in case no student comes to wait on a course during the `SLOTS_FILLING` time, the tutorial is normally carried out, with 0 participants by the selected TA.