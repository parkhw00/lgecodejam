
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <limits.h>

#if 1
#define THREADS	4
#define debug(f,a...)	do{}while(0)
#else
#define DEBUG
#define THREADS	1
#define debug(f,a...)	printf("%s.%d."f,__func__, __LINE__,##a)
#endif
#define error(f,a...)	printf("%s.%d."f,__func__, __LINE__,##a)


#define list_init(l)		{(l)->next = (l)->prev = (l);}
#define list_add_before(to,new)	{ \
	(to)->prev->next = (new); \
	(new)->prev = (to)->prev; \
	(to)->prev = (new); \
	(new)->next = (to); \
}
#define list_remove(l)	{ \
	(l)->prev->next = (l)->next; \
	(l)->next->prev = (l)->prev; \
	(l)->next = (l)->prev = NULL; \
}

struct city;

struct link
{
	struct link *next;

	int length;
	struct city *to;
};

struct city
{
	int number;
	struct link *link;
	struct link *link_down;
	int link_count;

	int search_distance;

	int distance_down;
	int distance_up;
	struct city *next;
	struct city *prev;
};

struct question
{
	struct city *c1, *c2;

	int cross_distance;
	int city1_distance_down;
	int city1_distance_up;
};

struct jobcontrol;

struct jobarg
{
	int number;
	int running;
	struct jobcontrol *jc;
	int result1;
	int result2;

	int N, Q;
	int M;
	int *distance_to1;
	struct question *question;
	struct city *city;
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

int search_shortest (struct jobarg *j, struct question *q)
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
	for (a=0; a<j->N; a++)
	{
		j->city[a].distance_down = -1;
		j->city[a].distance_up = -1;
	}

	/* down to city1 from start(c1) */
	c1->distance_down = 0;
	c = c1;
	while (c->link_down)
	{
		struct city *n;

		n = c->link_down->to;
		n->distance_down = c->distance_down + c->link_down->length;
		debug ("down %d -> %d(%d)\n", c->number+1, n->number+1, n->distance_down);

		c = n;
	}

	/* up to end(c2) from city1 */
	c2->distance_up = 0;
	if (c2->distance_down >= 0)
		cross = c2;
	c = c2;
	while (c->link_down)
	{
		struct city *n;

		n = c->link_down->to;
		n->distance_up = c->distance_up + c->link_down->length;
#if 0
		debug ("up %d -> %d(down:%d, up:%d)\n", c->number+1, n->number+1,
				n->distance_down,
				n->distance_up);
#endif

		if (!cross && n->distance_down >= 0)
			cross = n;

		c = n;
	}

	if (!cross)
	{
		error ("no path??\n");
		exit (1);
	}

	q->cross_distance = cross->distance_down + cross->distance_up;
	q->city1_distance_down = j->city[0].distance_down;
	q->city1_distance_up = j->city[0].distance_up;

	debug ("distance %d\n", q->cross_distance);
	return q->cross_distance;
}

int search_shortest2 (struct jobarg *j, struct question *q)
{
	int a;
	struct city *c1 = q->c1;
	struct city *c2 = q->c2;
	struct city *now;
	int shortest;
	int got_city1;

	debug ("search city%d to city%d\n", c1->number+1, c2->number+1);

	if (c1 == c2)
		return 0;

	/* reset parameter */
	for (a=0; a<j->N; a++)
	{
		j->city[a].distance_down = INT_MAX;
		j->city[a].next = j->city[a].prev =  NULL;
	}

	/* down to city1 from start(c1) */
	c1->distance_down = 0;
	now = c1;
	list_init (now);
	got_city1 = 0;
	while (1)
	{
		struct city *n;
		struct city *t;
		struct link *l;

#if 1
		if (now == j->city+0)
		{
			debug ("we got city1. break\n");
			t = now;
			while (1)
			{
				t = t->next;
				if (t == now)
					break;

				debug ("delete unclear distance city%d\n", t->number+1);
				t->distance_down = INT_MAX;
			}
			break;
		}
#endif
		debug ("spread from city%d\n", now->number+1);

		if (!got_city1 && now == c2)
		{
#if 1
			debug ("we got destination. distance %d\n",
					now->distance_down);
#endif
			return now->distance_down;
		}

#if 0
		if (now == j->city+0)
		{
			got_city1 = 1;
		}
		else
#endif
		{
			l = now->link;
			while (l)
			{
				struct city *leaf;
				int dist;

				leaf = l->to;
				dist = now->distance_down + l->length;

				if (leaf->distance_down < dist)
				{
					l = l->next;
					continue;
				}
				leaf->distance_down = dist;
				if (leaf->next)
					list_remove (leaf);

				debug ("leaf city%d(%d)\n", leaf->number+1, leaf->distance_down);

				t = now;
				while (t)
				{
					if (t->distance_down > dist)
						break;
					t = t->next;
					if (t == now)
						break;
				}
				list_add_before (t, leaf);

				l = l->next;
			}
		}

		n = now->next;
		list_remove (now);
		now = n;

		if (!now->next)
		{
			debug ("no more..\n");
			break;
		}

#ifdef DEBUG
		t = now;
		while (t)
		{
			debug ("current spread list. city%d(%d)\n",
					t->number+1, t->distance_down);

			t = t->next;
			if (t == now)
				break;
		}
#endif
	}

	/* reset parameter */
	for (a=0; a<j->N; a++)
	{
		j->city[a].distance_up = INT_MAX;
		j->city[a].next = j->city[a].prev =  NULL;
	}

	/* up to city2 from start(c1) */
	debug ("spread from backward...\n");
	c2->distance_up = 0;
	now = c2;
	list_init (now);
	shortest = INT_MAX;
	while (now)
	{
		struct city *n;
		struct city *t;
		struct link *l;

		debug ("spread from city%d\n", now->number+1);

#if 1
		if (now->distance_down != INT_MAX)
		{
			debug ("we got cross. distance %d, %d\n",
					now->distance_down,
					now->distance_up);
#if 0
			if (shortest > now->distance_down + now->distance_up)
				shortest = now->distance_down + now->distance_up;
#else
			shortest = INT_MAX;
			t = now;
			while (t)
			{
				int dist;

				if (t->distance_down != INT_MAX &&
						t->distance_up != INT_MAX)
				{
					dist = t->distance_down + t->distance_up;
					if (shortest > dist)
						shortest = dist;
				}

				t = t->next;
				if (t == now)
					break;
			}
			debug ("distance %d\n", shortest);
			return shortest;
#endif
		}
#endif

#if 0
		if (now == c1)
		{
			debug ("we got destination. distance %d\n",
					now->distance_up);
			return now->distance_up;
		}
#endif

#if 0
		if (shortest != INT_MAX && now->distance_down != INT_MAX)
			return shortest;
#endif

		if (now != j->city+0)
		{
			l = now->link;
			while (l)
			{
				struct city *leaf;
				int dist;

				leaf = l->to;
				dist = now->distance_up + l->length;

				if (leaf->distance_up < dist)
				{
					l = l->next;
					continue;
				}
				leaf->distance_up = dist;
				if (leaf->next)
					list_remove (leaf);

				debug ("leaf city%d(%d)\n", leaf->number+1, leaf->distance_up);

				t = now;
				while (t)
				{
					if (t->distance_up > dist)
						break;
					t = t->next;
					if (t == now)
						break;
				}
				list_add_before (t, leaf);

				l = l->next;
			}
		}

		n = now->next;
		list_remove (now);
		now = n;

		if (!now->next)
		{
			debug ("no more..\n");
			break;
		}

#ifdef DEBUG
		t = now;
		while (t)
		{
			debug ("current spread list. city%d(%d)\n",
					t->number+1, t->distance_up);

			t = t->next;
			if (t == now)
				break;
		}
#endif
	}

	shortest = INT_MAX;
	for (a=0; a<j->N; a++)
	{
		if (j->city[a].distance_down != INT_MAX &&
				j->city[a].distance_up != INT_MAX)
		{
			int dist;

			dist = j->city[a].distance_down +
				j->city[a].distance_up;

			if (shortest > dist)
			{
				c1 = j->city+a;
				shortest = dist;
			}
		}
	}

	debug ("shortest %d crossing city%d\n", shortest, c1->number+1);
	return shortest;
}

void * jobthread (void *arg)
{
	struct jobarg *j = arg;

	int a;
	int q, m;

	for (q=0; q<j->Q; q++)
		j->result1 +=
			search_shortest (j, j->question+q);

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

	for (q=0; q<j->Q; q++)
		j->result2 +=
			search_shortest2 (j, j->question+q);


	pthread_mutex_lock (&j->jc->lock);
	j->running = 0;
	printf ("%d. %d, %d\n", j->number, j->result1, j->result2);
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

		j->question[a].c1 = j->city+n1-1;
		j->question[a].c2 = j->city+n2-1;
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
		printf ("%d. result %d, %d\n", i, jc->arg[i].result1, jc->arg[i].result2);
		fprintf (jc->out, "%d %d\n", jc->arg[i].result1, jc->arg[i].result2);
	}

	return 0;
}

