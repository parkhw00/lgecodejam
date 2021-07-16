
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <limits.h>

#if 1
#define THREADS	4
#define debug(f,a...)	do{}while(0)
#else
//#define DEBUG
#define THREADS	1
#define debug(f,a...)	printf("%s.%d."f,__func__, __LINE__,##a)
#endif
#define error(f,a...)	printf("%s.%d."f,__func__, __LINE__,##a)


#define list_next(j,l)			(j->city_list[(l)->number].next)
#define list_prev(j,l)			(j->city_list[(l)->number].prev)
#define list_init(j,l)			{list_next(j,l) = list_prev(j,l) = (l);}
#define list_add_before(j,to,new)	{ \
	list_next(j,list_prev(j,to)) = (new); \
	list_prev(j,new) = list_prev(j,to); \
	list_prev(j,to) = (new); \
	list_next(j,new) = (to); \
}
#define list_remove(j, l)	{ \
	list_next(j,list_prev(j,l)) = list_next(j,l); \
	list_prev(j,list_next(j,l)) = list_prev(j,l); \
	list_next(j,l) = list_prev(j,l) = NULL; \
}

struct city;

struct link
{
	struct link *next;

	int length;
	struct city *to;
};

struct city_list
{
	struct city *next;
	struct city *prev;
};

struct city
{
	int number;
	struct link *link;
	struct link *link_down;
	int link_count;

	int search_distance;
};

struct question
{
	struct city *c1, *c2;

	int count;
};

struct jobcontrol;

struct jobarg
{
	int number;
	int running;
	struct jobcontrol *jc;
	unsigned long long result1;
	unsigned long long result2;

	int N, Q;
	int M;
	int *distance_to1;
	struct question *question;
	struct city *city;
	unsigned int *distance_down;
	unsigned int *distance_up;
	struct city_list *city_list;
};

struct jobcontrol
{
	FILE *in, *out;

	pthread_cond_t jobdone;
	pthread_mutex_t lock;

	int next_job;
	struct jobarg *arg;
	pthread_t *tid;
	int T;
};

int add_link (int length, struct city *c1, struct city *c2)
{
	struct link *link;

	c1->link_count ++;
	c2->link_count ++;

	link = calloc (sizeof (*link), 1);
	link->length = length;
	link->to = c1;
	link->next = c2->link;
	c2->link = link;
	c2->link_down = link;

	link = calloc (sizeof (*link), 1);
	link->length = length;
	link->to = c2;
	link->next = c1->link;
	c1->link = link;

	return 0;
}

int search_before1 (struct jobarg *j, struct question *q)
{
	int a;
	int leaf_num;
	struct city *c1 = q->c1;
	struct city *c2 = q->c2;
	struct city *cross = NULL;
	struct city *c;

	debug ("search city%d to city%d\n", c1->number+1, c2->number+1);

	if (c1 == c2)
		return 0;

	/* reset parameter */
	memset (j->distance_down, 0xff, sizeof(j->distance_down[0])*j->N);

	/* down to city1 from start(c1) */
	j->distance_down[c1->number] = 0;
	c = c1;
	while (c->link_down)
	{
		struct city *n;

		n = c->link_down->to;
		j->distance_down[n->number] =
			j->distance_down[c->number] + c->link_down->length;

		c = n;
	}

	return 0;
}

int search_before2 (struct jobarg *j, struct question *q)
{
	int a;
	int leaf_num;
	struct city *c1 = q->c1;
	struct city *c2 = q->c2;
	struct city *c;
	int distance;

	debug ("search city%d to city%d\n", c1->number+1, c2->number+1);

	if (c1 == c2)
		return 0;

	if (j->distance_down[c2->number] != UINT_MAX)
		return j->distance_down[c2->number];

	/* up to end(c2) from city1 */
	distance = 0;
	c = c2;
	while (c->link_down)
	{
		struct city *n;

		n = c->link_down->to;
		distance += c->link_down->length;

		if (j->distance_down[n->number] != UINT_MAX)
			return j->distance_down[n->number] + distance;

		c = n;
	}

	error ("no path??\n");
	exit (1);
}

int search_step1 (struct jobarg *j, struct question *q)
{
	int a;
	struct city *c1 = q->c1;
	struct city *c2 = q->c2;
	struct city *now;

	debug ("search city%d to city%d\n", c1->number+1, c2->number+1);

	if (c1 == c2)
		return 0;

	/* reset parameter */
	memset (j->city_list, 0, sizeof (j->city_list[0]) * j->N);
	memset (j->distance_down, 0xff, sizeof (j->distance_down[0]) * j->N);

	/* down to city1 from start(c1) */
	j->distance_down[c1->number] = 0;
	now = c1;
	list_init (j, now);
	while (1)
	{
		struct city *n;
		struct city *t;
		struct link *l;

#if 1
		if (now == j->city+0)
#else
		if (0)
#endif
		{
			debug ("we got city1. break\n");
			t = now;
			while (1)
			{
				t = list_next (j, t);
				if (t == now)
					break;

				debug ("delete unclear distance city%d\n", t->number+1);
				j->distance_down[t->number] = UINT_MAX;
			}

			break;
		}
		debug ("spread from city%d\n", now->number+1);

		if (now == c2)
		{
			debug ("we got destination. distance %d\n",
					j->distance_down[now->number]);
			return j->distance_down[now->number];
		}

		l = now->link;
		while (l)
		{
			struct city *leaf;
			int dist;

			leaf = l->to;
			dist = j->distance_down[now->number] + l->length;

			if (j->distance_down[leaf->number] < dist)
			{
				l = l->next;
				continue;
			}
			j->distance_down[leaf->number] = dist;
			if (list_next (j, leaf))
				list_remove (j, leaf);

			debug ("leaf city%d(%d)\n", leaf->number+1, j->distance_down[leaf->number]);

			t = now;
			while (t)
			{
				if (j->distance_down[t->number] > dist)
					break;
				t = list_next (j, t);
				if (t == now)
					break;
			}
			list_add_before (j, t, leaf);

			l = l->next;
		}

		n = list_next (j, now);
		list_remove (j, now);
		now = n;

		if (!list_next (j, now))
		{
			error ("no path??\n");
			exit (1);
		}

#ifdef DEBUG
		t = now;
		while (t)
		{
			debug ("current spread list. city%d(%d)\n",
					t->number+1, j->distance_down[t->number]);

			t = list_next (j, t);
			if (t == now)
				break;
		}
#endif
	}

	debug ("need step2\n");
	return -1;
}

int search_step2 (struct jobarg *j, struct question *q)
{
	int a;
	struct city *c1 = q->c1;
	struct city *c2 = q->c2;
	struct city *now;

	debug ("search city%d to city%d\n", c1->number+1, c2->number+1);

	/* reset parameter */
	memset (j->city_list, 0, sizeof (j->city_list[0]) * j->N);
	memset (j->distance_up, 0xff, sizeof (j->distance_up[0]) * j->N);

	/* up to city2 from start(c1) */
	debug ("spread from backward...\n");
	j->distance_up[c2->number] = 0;
	now = c2;
	list_init (j, now);
	while (now)
	{
		struct city *n;
		struct city *t;
		struct link *l;

		debug ("spread from city%d\n", now->number+1);

		if (j->distance_down[now->number] != UINT_MAX)
		{
			int shortest;

			debug ("we got cross. distance %d, %d\n",
					j->distance_down[now->number],
					j->distance_up[now->number]);
			shortest = INT_MAX;
			t = now;
			while (t)
			{
				int dist;

				if (j->distance_down[t->number] != UINT_MAX &&
						j->distance_up[t->number] != UINT_MAX)
				{
					dist = j->distance_down[t->number] + j->distance_up[t->number];
					if (shortest > dist)
						shortest = dist;
				}

				t = j->city_list[t->number].next;
				if (t == now)
					break;
			}
			debug ("distance %d\n", shortest);
			return shortest;
		}

		if (now != j->city+0)
		{
			l = now->link;
			while (l)
			{
				struct city *leaf;
				int dist;

				leaf = l->to;
				dist = j->distance_up[now->number] + l->length;

				if (j->distance_up[leaf->number] < dist)
				{
					l = l->next;
					continue;
				}
				j->distance_up[leaf->number] = dist;
				if (j->city_list[leaf->number].next)
					list_remove (j, leaf);

				debug ("leaf city%d(%d)\n",
						leaf->number+1, j->distance_up[leaf->number]);

				t = now;
				while (t)
				{
					if (j->distance_up[t->number] > dist)
						break;
					t = j->city_list[t->number].next;
					if (t == now)
						break;
				}
				list_add_before (j, t, leaf);

				l = l->next;
			}
		}

		n = j->city_list[now->number].next;
		list_remove (j, now);
		now = n;

		if (!j->city_list[now->number].next)
		{
			debug ("no more..\n");
			break;
		}

#ifdef DEBUG
		t = now;
		while (t)
		{
			debug ("current spread list. city%d(%d)\n",
					t->number+1, j->distance_up[t->number]);

			t = j->city_list[t->number].next;
			if (t == now)
				break;
		}
#endif
	}

	error ("no path?\n");
	exit (1);
}

void * jobthread (void *arg)
{
	struct jobarg *j = arg;

	int a;
	int q, m;

	int step1_done;
	int pre_c1_num;

	/* before bridge construction */
	step1_done = 0;
	pre_c1_num = -1;
	for (q=0; q<j->Q; q++)
	{
		int dist;

		debug ("before check city%d, city%d, step1done%d\n",
				j->question[q].c1->number+1,
				j->question[q].c2->number+1,
				step1_done);

		if (pre_c1_num != j->question[q].c1->number)
			step1_done = 0;

		if (j->question[q].c1 == j->question[q].c2)
			dist = 0;
		else
		{
			if (!step1_done)
			{
				search_before1 (j, j->question+q);
				step1_done = 1;
			}

			dist = search_before2 (j, j->question+q);
		}

		j->result1 += dist*j->question[q].count;

		pre_c1_num = j->question[q].c1->number;
	}

	/* bridge construction */
	m = 0;
	for (a=1; a<j->N; a++)
	{
		struct city *c = j->city+a;

		if (c->link_count == 1)
		{
			if (c->link->to == j->city+0 &&
					c->link->length > j->distance_to1[m])
			{
				struct link *l;

				c->link->length = j->distance_to1[m];
				l = j->city[0].link;
				while (l)
				{
					if (l->to == c)
					{
						l->length = j->distance_to1[m];
						break;
					}
					l = l->next;
				}
				if (!l)
				{
					error ("Oops\n");
					exit (1);
				}
			}
			else
			{
				debug ("add link 1 -- %d dis %d\n", a+1, j->distance_to1[m]);
				add_link (j->distance_to1[m], j->city+0, j->city+a);
			}

			m++;
		}
	}

	/* after */
	step1_done = 0;
	pre_c1_num = -1;
	for (q=0; q<j->Q; q++)
	{
		int dist;

		debug ("after check city%d, city%d, step1done%d\n",
				j->question[q].c1->number+1,
				j->question[q].c2->number+1,
				step1_done);

		if (pre_c1_num != j->question[q].c1->number)
			step1_done = 0;

		if (j->question[q].c1 == j->question[q].c2)
			dist = 0;
		else
		{
			dist = -1;
			if (!step1_done)
			{
				dist = search_step1 (j, j->question+q);
				if (dist < 0)
					step1_done = 1;
			}

			if (dist < 0)
				dist = search_step2 (j, j->question+q);
		}

		j->result2 += dist*j->question[q].count;

		pre_c1_num = j->question[q].c1->number;
	}


	pthread_mutex_lock (&j->jc->lock);
	j->running = 0;
	printf ("%d. %llu, %llu\n", j->number, j->result1, j->result2);
	pthread_mutex_unlock (&j->jc->lock);
	pthread_cond_signal (&j->jc->jobdone);

	for (a=0; a<j->N; a++)
	{
		struct link *l, *n;

		l = j->city[a].link;
		while (l)
		{
			n = l->next;
			free (l);

			l = n;
		}
	}
	free (j->city);
	free (j->distance_to1);
	free (j->question);

	return 0;
}

int comp_question (const void *a1, const void *a2)
{
	struct question *q1 = (void*)a1;
	struct question *q2 = (void*)a2;

	if (q1->c1->number == q2->c1->number)
		return q1->c2->number - q2->c2->number;

	return q1->c1->number - q2->c1->number;
}

int more_job (struct jobcontrol *jc)
{
	struct jobarg *j;
	int jn;

	int a;

	if (jc->next_job >= jc->T)
		return 0;
	jn = jc->next_job ++;

	j = jc->arg + jn;

	fscanf (jc->in, "%d %d\n", &j->N, &j->Q);
	printf ("%d. new job. N %d, Q %d\n", jn, j->N, j->Q);

	j->city = calloc (j->N, sizeof (j->city[0]));
	j->distance_down = calloc (j->N, sizeof (j->distance_down[0]));
	j->distance_up = calloc (j->N, sizeof (j->distance_up[0]));
	j->city_list = calloc (j->N, sizeof (j->city_list[0]));

	for (a=0; a<j->N; a++)
	{
		int num, dis;

		j->city[a].number = a;
		if (a == 0)
			continue;

		num = dis = 0;
		fscanf (jc->in, "%d %d\n", &num, &dis);
		debug ("link %d -- %d, %d\n", a+1, num, dis);

		add_link (dis, j->city+num-1, j->city+a);
	}

	for (a=1; a<j->N; a++)
		if (j->city[a].link_count == 1)
			j->M ++;

	debug ("M %d\n", j->M);
	j->distance_to1 = calloc (j->M, sizeof (int));
	for (a=0; a<j->M; a++)
	{
		fscanf (jc->in, "%d\n", j->distance_to1+a);
		debug ("m %d, %d\n", a, j->distance_to1[a]);
	}

	j->question = calloc (j->Q, sizeof (j->question[0]));
	for (a=0; a<j->Q; a++)
	{
		int n1, n2;

		n1 = n2 = 0;
		fscanf (jc->in, "%d %d\n", &n1, &n2);
		debug ("q %d, %d, %d\n", a, n1, n2);
		if (n1 > j->N || n2 > j->N)
		{
			error ("Oops\n");
			exit (1);
		}

		if (n1 < n2)
		{
			int t;

			t = n1;
			n1 = n2;
			n2 = t;
		}

		j->question[a].c1 = j->city+n1-1;
		j->question[a].c2 = j->city+n2-1;
		j->question[a].count = 1;

	}
	{
		int q_count;
		struct question *pre_q;

		/* sort questions to speed up */
		qsort (j->question, j->Q, sizeof (j->question[0]),
				comp_question);

#if 0
		/* count and remove same questions */
		pre_q = j->question+0;
		q_count = 1;
		for (a=1; a<j->Q; a++)
		{
			if (
					pre_q->c1 == j->question[a].c1 &&
					pre_q->c2 == j->question[a].c2
			   )
			{
				pre_q->count ++;
			}
			else
			{
				q_count ++;
				pre_q ++;
				*pre_q = j->question[a];
			}
		}

		printf ("question %d to %d\n", j->Q, q_count);
		j->Q = q_count;
#endif
	}

	j->number = jn;
	j->running = 1;
	j->jc = jc;

	pthread_create (jc->tid+jn, NULL, jobthread, jc->arg+jn);
	pthread_detach (jc->tid[jn]);

	return 0;
}

int now_running (struct jobcontrol *jc)
{
	int i;
	int running = 0;

	for (i=0; i<jc->T; i++)
		if (jc->arg[i].running)
			running ++;

	return running;
}

int main (int argc, char **argv)
{
	char *_in, *_out, *p;
	struct jobcontrol _jc = {0,};
	struct jobcontrol *jc = &_jc;
	int i;

	if (argc < 2)
	{
		error ("no input\n");
		return 1;
	}

	_in = argv[1];
	_out = malloc (strlen (_in) + 5);
	strcpy (_out, _in);
	p = strrchr (_out, '.');
	if (!p)
		p = _out+strlen (_in);
	strcpy (p, ".out");

	debug ("in %s, out %s\n", _in, _out);
	jc->in = fopen (_in, "r");
	jc->out = fopen (_out, "w");
	if (!jc->in || !jc->out)
	{
		error ("no input or output\n");
		return 1;
	}

	pthread_mutex_init (&jc->lock, NULL);
	pthread_cond_init (&jc->jobdone, NULL);

	fscanf (jc->in, "%d\n", &jc->T);
	printf ("T %d\n", jc->T);

	jc->arg = calloc (jc->T, sizeof (jc->arg[0]));
	jc->tid = calloc (jc->T, sizeof (jc->tid[0]));
	if (!jc->arg || !jc->tid)
	{
		error ("no mem for arg or tid\n");
		return 1;
	}

	while (1)
	{
		int running;

		pthread_mutex_lock (&jc->lock);
		running = now_running (jc);
		if (running >= THREADS)
			pthread_cond_wait (&jc->jobdone, &jc->lock);
		else if (jc->next_job >= jc->T)
		{
			if (running == 0)
			{
				debug ("all done.\n");
				pthread_mutex_unlock (&jc->lock);
				break;
			}

			pthread_cond_wait (&jc->jobdone, &jc->lock);
		}
		pthread_mutex_unlock (&jc->lock);

		more_job (jc);
	}

	for (i=0; i<jc->T; i++)
	{
		printf ("%d. result %llu, %llu\n", i, jc->arg[i].result1, jc->arg[i].result2);
		fprintf (jc->out, "%llu %llu\n", jc->arg[i].result1, jc->arg[i].result2);
	}

	return 0;
}

