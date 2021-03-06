rm(list = ls())
library(igraph) # visualization of trees and solution. 
library(amap)   # compute different distance https://www.rdocumentation.org/packages/amap/versions/0.8-16/topics/Dist

# Compute distance between two vector for different distance measures
distance_fun <- function(x, y, method = "euclidean"){
  A <- matrix(c(as.vector(unlist(x)), as.vector(unlist(y))), nrow=2,byrow=T)
  return(as.numeric(Dist(A, method = method)))
}

# create a Trajan constructor
Trajan <- function (t1, t1_root = NULL, t2, t2_root = NULL, t1_data = NULL, t2_data = NULL, distance_matrix = NULL, penalty = "avg", method = "euclidean"){
  # Convert to rooted tree given the undirected graph with root.
  if (!is.null(t1_root)){
    g1 <- graph_from_edgelist(as.matrix(t1))
    g1_dist <- bfs(graph = g1, root = t1_root, neimode = "all", dist = TRUE)
    g1_dist <- g1_dist$dist
    for (i in 1: nrow(t1)){
      source <- toString(t1[i,1])
      target <- toString(t1[i,2])
      if (g1_dist[source] <= g1_dist[target]){
        t1[i,1] <- target
        t1[i,2] <- source
      }
    }
  }
  if(!is.null(t2_root)){
    g2 <- graph_from_edgelist(as.matrix(t2))
    g2_dist <- bfs(graph = g2, root = t2_root, neimode = "all", dist = TRUE)
    g2_dist <- g2_dist$dist
    for (i in 1:nrow(t2)){
      source <- toString(t2[i,1])
      target <- toString(t2[i,2])
      if (g2_dist[source] <= g2_dist[target]){
        t2[i,1] <- target
        t2[i,2] <- source
      }
    }
  }
  
  value = list(t1 = t1, t1_root = t1_root, t2 = t2, t2_root = t2_root, t1_data = t1_data, t2_data = t2_data, 
               distance_matrix = distance_matrix, penalty = penalty, method = method)
  attr(value, "class") <- "Trajan"
  return(value)
}

# export an input for Trajan.
export = function(obj, t1_treefName = "t1.tree", t1_mapfName = "t1.map", 
                  t2_treefName = "t2.tree", t2_mapfName = "t2.map", distance_matrixfName = "distance_matrix.csv") {
  UseMethod("export", obj)
}
export.Trajan <- function(obj, t1_treefName = "t1.tree", t1_mapfName = "t1.map", 
                          t2_treefName = "t2.tree", t2_mapfName = "t2.map", distance_matrixfName = "distance_matrix.csv")
{
  #Compute distance matrix if it is not given
  n <- nrow(obj$t1)+1
  m <- nrow(obj$t2)+1
  if (is.null(obj$distance_matrix)){
    t1_nodes <- colnames(obj$t1_data)
    t2_nodes <- colnames(obj$t2_data)
    obj$distance_matrix <- data.frame(matrix(0, ncol = m, nrow = n))
    colnames(obj$distance_matrix) <- t2_nodes
    rownames(obj$distance_matrix) <- t1_nodes
    for (i in 1: n){
      for (j in 1: m) {
        obj$distance_matrix[i, j] <- distance_fun(obj$t1_data[t1_nodes[i]], obj$t2_data[t2_nodes[j]], obj$method)
      }
    }
    print("Compute distance matrix")
  }
  
  # Export the two trees.
  t1_edge_set <- obj$t1
  t1_edge_set$last <- rep("default", each= n-1)
  t2_edge_set <- obj$t2
  t2_edge_set$last <- rep("default", each = m-1)
  write.table(t1_edge_set, file = t1_treefName, sep = ' ', row.names=FALSE, col.names = FALSE, quote = FALSE)
  write.table(t2_edge_set, file = t2_treefName, sep = ' ', row.names=FALSE, col.names = FALSE, quote = FALSE)
  t1_nodes <- rownames(obj$distance_matrix)
  t2_nodes <- colnames(obj$distance_matrix)
  has_penalty <- FALSE
  if (tail(t1_nodes, n=1) == "penalty" || tail(t2_nodes, n=1) == "penalty")
  {
    has_penalty <- TRUE
    t1_nodes <- t1_nodes[1:(length(t1_nodes)-1)]
    t2_nodes <- t2_nodes[1:(length(t2_nodes)-1)]
  }

  # Export the mappings
  t1_map_df <- data.frame(labels = 0:(length(t1_nodes)-1), nodes = t1_nodes)
  write.table(t1_map_df, file = t1_mapfName, sep = ' ', row.names=FALSE, col.names = FALSE, quote = FALSE)
  t2_map_df <- data.frame(labels = 0:(length(t2_nodes)-1), nodes = t2_nodes)
  write.table(t2_map_df, file = t2_mapfName, sep = ' ', row.names=FALSE, col.names = FALSE, quote = FALSE)

  # Export the distance matrix
  if (has_penalty == FALSE){
    if (obj$penalty == "avg"){
      avg_distance_matrix <- obj$distance_matrix
      avg_distance_matrix$last <- rowMeans(obj$distance_matrix)
      avg_distance_matrix[nrow(avg_distance_matrix)+1,] = c(colMeans(obj$distance_matrix),0)
      write.table(avg_distance_matrix, file = distance_matrixfName, sep = ',', row.names=FALSE, col.names = FALSE)
    }
    else if (obj$penalty == "max"){
      max_distance_matrix <- obj$distance_matrix
      max_distance_matrix$last <- apply(obj$distance_matrix, 1, FUN = max)
      max_distance_matrix[nrow(max_distance_matrix)+1,] = c(apply(obj$distance_matrix, 2, FUN = max),0)
      write.table(max_distance_matrix, file = distance_matrixfName ,sep = ',', row.names=FALSE, col.names = FALSE)
    }
    else
    {
      print("Penalty is undefined!!")
    }
  }
  else
  {
    n <- nrow(obj$distance_matrix)
    m <- ncol(obj$distance_matrix)
    obj$distance_matrix[n, m] = 0 # add a redandunt value
    write.table(obj$distance_matrix, file = distance_matrixfName ,sep = ',', row.names=FALSE, col.names = FALSE)
  }
}

plot_first_tree = function(obj) {
  UseMethod("plot_first_tree", obj)
}
plot_first_tree.Trajan <- function(obj){
  igraph_options(plot.layout=layout_as_tree)
  g1_edges <- obj$t1
  edge_set <- data.frame(source = g1_edges[,2], target = g1_edges[,1])
  g1 <- graph_from_edgelist(as.matrix(edge_set))
  
  pdf("first_tree.pdf", 50,50)
  plot(g1, vertex.size=2,  edge.arrow.size=1) #, vertex.size=3, vertex.label=T, edge.arrow.size=0.6
  dev.off()
}

plot_second_tree = function(obj) {
  UseMethod("plot_second_tree", obj)
}
plot_second_tree <- function(obj){
  igraph_options(plot.layout=layout_as_tree)
  g2_edges <- obj$t2
  edge_set <- data.frame(source = g2_edges[,2], target = g2_edges[,1])
  g2 <- graph_from_edgelist(as.matrix(edge_set))
  pdf("second_tree.pdf", 50,50)
  plot(g2, vertex.size=2,  edge.arrow.size=1)
  dev.off()
}

plot_solution = function(obj, optimal_path){
  UseMethod("plot_solution", obj)
}
plot_solution <- function(obj, optimal_path){
  pdf("optimal_solution.pdf", 50,50)
  g1_edges <- obj$t1
  edge_set1 <- data.frame(source = g1_edges[,2], target = g1_edges[,1])
  g1 <- graph_from_edgelist(as.matrix(edge_set1))
  E(g1)$layout <- 1
  E(g1)$color <- "red"
                            
  g2_edges <- obj$t2
  edge_set2 <- data.frame(source = g2_edges[,2], target = g2_edges[,1])
  g2 <- graph_from_edgelist(as.matrix(edge_set2))
  E(g2)$layout <- 10
  E(g2)$color <- "blue"
  
  union_graph <- graph.union(g1, g2, byname = "auto")
  
  for (i in 1:nrow(optimal_path)){
    union_graph <- union_graph %>% add_edges(c(toString(optimal_path[i,1]), toString(optimal_path[i,2]))) # %>%set_edge_attr("arrow.size", value = 0.1)
  }
  # plot(union_graph, edge.arrow.size=0,vertex.shape="none", add=TRUE)
  color_attr <- c()
  weight_attr <- c()
  n <- nrow(obj$t2)
  m <- nrow(obj$t1)
  for (i in 1: n){
    color_attr <- c(color_attr, "blue")
    weight_attr <- c(weight_attr, 1)
  }

  for (i in 1: m){
    color_attr <- c(color_attr, "green")
    weight_attr <- c(weight_attr, 1)
  }

  for (i in 1: nrow(optimal_path)){
    color_attr <- c(color_attr, "red")
    weight_attr <- c(weight_attr, 5)
  }
  igraph_options(plot.layout=layout_as_tree)
  union_graph <- union_graph%>%set_edge_attr("arrow.width", value = 0.5) %>% set_edge_attr("color", value = color_attr)
  
  plot(union_graph,  edge.width = weight_attr, vertex.size=2,  edge.arrow.size=0.8)
  dev.off()
}


