#ifndef _NANOCLJ_GRAPH_H_
#define _NANOCLJ_GRAPH_H_

#include <math.h>

#define EPSILON 0.00000001

// Gauss-Seidel relaxation for links
static inline void relax_links(nanoclj_cell_t * g, float alpha) {
  size_t num_nodes = g->_graph.num_nodes;
  size_t num_edges = g->_graph.num_edges;
  nanoclj_graph_array_t * ga = g->_graph.rep;
  
  for (size_t i = 0; i < num_edges; i++) {
    nanoclj_edge_t * e = &(ga->edges[g->_graph.edge_offset + i]);
    int tail = e->source, head = e->target;
    if (tail == head) continue;
    if (tail < g->_graph.node_offset || tail >= g->_graph.node_offset + g->_graph.num_nodes) continue;
    if (head < g->_graph.node_offset || head >= g->_graph.node_offset + g->_graph.num_nodes) continue;
    
    float edge_weight = 1, w1 = 1, w2 = 1;
    
    nanoclj_node_t * n1 = &(ga->nodes[tail]), * n2 = &(ga->nodes[head]);
    nanoclj_vec2f d = n2->pos;
    d.x -= n1->pos.x;
    d.y -= n1->pos.y;
    float l = sqrtf(d.x*d.x + d.y*d.y);
    
    if (l < EPSILON) continue;

    d.x /= l;
    d.y /= l;
    
    l *= alpha * edge_weight;

    float k = w1 / (w1 + w2);
    n2->pos.x -= d.x * l * k;
    n2->pos.y -= d.y * l * k;
    n1->pos.x += d.x * l * (1 - k);
    n1->pos.y += d.y * l * (1 - k);
  }
}

static inline void apply_drag(nanoclj_cell_t * g, float friction) {
  nanoclj_graph_array_t * ga = g->_graph.rep;
  for (int i = 0; i < g->_graph.num_nodes; i++) {
    nanoclj_node_t * n = &(ga->nodes[g->_graph.node_offset + i]);
    nanoclj_vec2f pos = n->pos;
    pos.x -= friction * (n->ppos.x - pos.x);
    pos.y -= friction * (n->ppos.y - pos.y);
    n->ppos = n->pos;
    n->pos = pos;
  }
}

static inline void apply_gravity(nanoclj_cell_t * g, float alpha, float gravity) {
  float k = alpha * gravity;
  if (k < EPSILON) return;
  nanoclj_graph_array_t * ga = g->_graph.rep;
  for (int i = 0; i < g->_graph.num_nodes; i++) {
    nanoclj_node_t * n = &(ga->nodes[g->_graph.node_offset + i]);
    float weight = 1.0f;
    nanoclj_vec2f pos = n->pos;
    float d = sqrtf(pos.x * pos.x + pos.y * pos.y);
    if (d > 0.001) {
      // pd.position -= pos * (k * sqrtf(d) / d * weight);
      n->pos.x -= pos.x * k * weight;
      n->pos.y -= pos.y * k * weight;
    }
  }
}

static inline void apply_repulsion(nanoclj_cell_t * g, float alpha, float charge) {
  size_t num_nodes = g->_graph.num_nodes;
  nanoclj_graph_array_t * ga = g->_graph.rep;
  float k = alpha * charge;

  for (size_t i = 0; i < num_nodes; i++) {
    nanoclj_node_t * n1 = &(ga->nodes[g->_graph.node_offset + i]);
    float weight1 = 1.0f;
    for (size_t j = i + 1; j < num_nodes; j++) {
      nanoclj_node_t * n2 = &(ga->nodes[g->_graph.node_offset + j]);
      float weight2 = 1.0f;
      nanoclj_vec2f d = n2->pos;
      d.x -= n1->pos.x;
      d.y -= n1->pos.y;
      float mag2 = d.x * d.x + d.y * d.y;
      if (mag2 < 0.1) mag2 = 0.1;
      n1->ppos.x -= d.x * k / mag2 / weight1;
      n1->ppos.y -= d.y * k / mag2 / weight1;
      n2->ppos.x += d.x * k / mag2 / weight2;
      n2->ppos.y += d.y * k / mag2 / weight2;
    }
  }
}

#endif
