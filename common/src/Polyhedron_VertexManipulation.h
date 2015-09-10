/*
 Copyright (C) 2010-2014 Kristian Duske
 
 This file is part of TrenchBroom.
 
 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TrenchBroom_Polyhedron_VertexManipulation_h
#define TrenchBroom_Polyhedron_VertexManipulation_h

#include <algorithm>

template <typename T, typename FP>
struct Polyhedron<T,FP>::MoveVerticesResult {
    typename V::List movedVertices;
    typename V::List deletedVertices;
    typename V::List unchangedVertices;
    typename V::List newVertexPositions;
    typename V::List unknownVertices;
    
    MoveVerticesResult() {}
    MoveVerticesResult(const typename V::List& i_movedVertices) :
    movedVertices(i_movedVertices) {}
    
    void add(const MoveVertexResult& result) {
        switch (result.type) {
            case MoveVertexResult::Type_VertexMoved:
                movedVertices.push_back(result.originalPosition);
                newVertexPositions.push_back(result.vertex->position());
                break;
            case MoveVertexResult::Type_VertexDeleted:
                deletedVertices.push_back(result.originalPosition);
                break;
            case MoveVertexResult::Type_VertexUnchanged:
                unchangedVertices.push_back(result.originalPosition);
                break;
            DEFAULT_SWITCH()
        }
    }
    
    void addUnknown(const V& position) {
        unknownVertices.push_back(position);
    }
    
    bool allVerticesMoved() const {
        return !hasDeletedVertices() && !hasUnchangedVertices() && unknownVertices.empty() && !newVertexPositions.empty();
    }
    
    bool hasDeletedVertices() const {
        return !deletedVertices.empty();
    }
    
    bool hasUnchangedVertices() const {
        return !unchangedVertices.empty();
    }
    
    bool hasUnknownVertices() const {
        return !unknownVertices.empty();
    }
};

template <typename T, typename FP>
typename Polyhedron<T,FP>::MoveVerticesResult Polyhedron<T,FP>::moveVertices(const typename V::List& positions, const V& delta, const bool allowMergeIncidentVertices) {
    Callback c;
    return moveVertices(positions, delta, allowMergeIncidentVertices, c);
}

template <typename T, typename FP> template <typename C>
typename Polyhedron<T,FP>::MoveVerticesResult Polyhedron<T,FP>::moveVertices(typename V::List positions, const V& delta, const bool allowMergeIncidentVertices, C& callback) {
    assert(checkInvariant());
    if (delta.null())
        return MoveVerticesResult(positions);
    const MoveVerticesResult result = doMoveVertices(positions, delta, allowMergeIncidentVertices, callback);
    assert(checkInvariant());
    return result;
}

template <typename T, typename FP>
typename Polyhedron<T,FP>::MoveVerticesResult Polyhedron<T,FP>::splitEdge(const V& v1, const V& v2, const V& delta) {
    Callback c;
    return splitEdge(v1, v2, delta, c);
}

template <typename T, typename FP> template <typename C>
typename Polyhedron<T,FP>::MoveVerticesResult Polyhedron<T,FP>::splitEdge(const V& v1, const V& v2, const V& delta, C& callback) {
    assert(checkInvariant());
    
    if (delta.null())
        return MoveVerticesResult();
    
    const SplitResult splitResult = splitEdge(v1, v2, callback);
    if (!splitResult.success)
        return MoveVerticesResult();
    
    const MoveVerticesResult moveResult = doMoveVertices(typename V::List(1, splitResult.vertexPosition), delta, false, callback);
    assert(checkInvariant());
    return moveResult;
}

template <typename T, typename FP>
typename Polyhedron<T,FP>::MoveVerticesResult Polyhedron<T,FP>::splitFace(const typename V::List& vertexPositions, const V& delta) {
    Callback c;
    return splitFace(vertexPositions, delta, c);
}

template <typename T, typename FP> template <typename C>
typename Polyhedron<T,FP>::MoveVerticesResult Polyhedron<T,FP>::splitFace(const typename V::List& vertexPositions, const V& delta, C& callback) {
    assert(checkInvariant());
    
    if (delta.null())
        return MoveVerticesResult();
    
    const SplitResult splitResult = splitFace(vertexPositions, callback);
    if (!splitResult.success)
        return MoveVerticesResult();
    
    const MoveVerticesResult moveResult = doMoveVertices(typename V::List(1, splitResult.vertexPosition), delta, false, callback);
    assert(checkInvariant());
    return moveResult;
}

template <typename T, typename FP> template <typename C>
typename Polyhedron<T,FP>::MoveVerticesResult Polyhedron<T,FP>::doMoveVertices(typename V::List positions, const V& delta, const bool allowMergeIncidentVertices, C& callback) {
    std::sort(positions.begin(), positions.end(), typename V::InverseDotOrder(delta));
    MoveVerticesResult totalResult;
    
    for (size_t i = 0; i < positions.size(); ++i) {
        Vertex* vertex = findVertexByPosition(positions[i]);
        if (vertex == NULL) {
            totalResult.addUnknown(positions[i]);
        } else {
            const V destination = vertex->position() + delta;
            const MoveVertexResult currentResult = moveVertex(vertex, destination, allowMergeIncidentVertices, callback);
            totalResult.add(currentResult);
        }
    }
    
    updateBounds();
    return totalResult;
}

template <typename T, typename FP>
struct Polyhedron<T,FP>::SplitResult {
    bool success;
    V vertexPosition;
    
    SplitResult(bool i_success, const V& i_vertexPosition = V::Null) :
    success(i_success),
    vertexPosition(i_vertexPosition) {}
};

template <typename T, typename FP> template <typename C>
typename Polyhedron<T,FP>::SplitResult Polyhedron<T,FP>::splitEdge(const V& v1, const V& v2, C& callback) {
    Edge* edge = findEdgeByPositions(v1, v2);
    if (edge == NULL)
        return SplitResult(false);
    
    Edge* newEdge = edge->splitAtCenter();
    m_edges.append(newEdge, 1);
    
    Vertex* newVertex = newEdge->firstVertex();
    m_vertices.append(newVertex, 1);
    
    return SplitResult(true, newVertex->position());
}

template <typename T, typename FP> template <typename C>
typename Polyhedron<T,FP>::SplitResult Polyhedron<T,FP>::splitFace(const typename V::List& vertexPositions, C& callback) {
    Face* face = findFaceByPositions(vertexPositions);
    if (face == NULL)
        return SplitResult(false);
    
    Vertex* newVertex = new Vertex(face->center());
    m_vertices.append(newVertex, 1);
    
    const size_t vertexCount = face->vertexCount();
    HalfEdge* current = face->boundary().front();
    
    // First, create a new triangle that cuts into the face. The face will be convex until the following loop finishes.
    {
        HalfEdge* next = current->next();
        
        HalfEdge* fromCenter = new HalfEdge(newVertex);
        HalfEdge* fromCenterTwin = new HalfEdge(current->origin());
        HalfEdge* toCenter = new HalfEdge(current->destination());
        HalfEdge* toCenterTwin = new HalfEdge(newVertex);
        
        HalfEdgeList boundaryReplacement;
        boundaryReplacement.append(fromCenterTwin, 1);
        boundaryReplacement.append(toCenterTwin, 1);
        face->replaceBoundary(current, current, fromCenterTwin);
        
        HalfEdgeList newFaceBoundary;
        newFaceBoundary.append(fromCenter, 1);
        newFaceBoundary.append(current, 1);
        newFaceBoundary.append(toCenter, 1);
        
        Face* newFace = new Face(newFaceBoundary);
        m_faces.append(newFace, 1);
        m_edges.append(new Edge(fromCenter, fromCenterTwin), 1);
        m_edges.append(new Edge(toCenter, toCenterTwin), 1);
        
        callback.faceWasSplit(face, newFace);
        current = next;
    }
    
    // Now just chop off more triangles until only triangles remain.
    for (size_t i = 0; i < vertexCount - 2; ++i) {
        HalfEdge* next = current->next();
        chopFace(face, current, callback);
        current = next;
    }
    
    return SplitResult(true, newVertex->position());
}

template <typename T, typename FP>
struct Polyhedron<T,FP>::MoveVertexResult {
    typedef enum {
        Type_VertexMoved,
        Type_VertexDeleted,
        Type_VertexUnchanged
    } Type;
    
    const Type type;
    V originalPosition;
    Vertex* vertex;
    
    MoveVertexResult(const Type i_type, const V& i_originalPosition, Vertex* i_vertex = NULL) :
    type(i_type),
    originalPosition(i_originalPosition),
    vertex(i_vertex) {
        assert(type != Type_VertexDeleted || vertex == NULL);
    }
    
    bool moved() const     { return type == Type_VertexMoved; }
    bool deleted() const   { return type == Type_VertexDeleted; }
    bool unchanged() const { return type == Type_VertexUnchanged; }
};

template <typename T, typename FP> template <typename C>
typename Polyhedron<T,FP>::MoveVertexResult Polyhedron<T,FP>::moveVertex(Vertex* vertex, const V& destination, const bool allowMergeIncidentVertex, C& callback) {
    assert(vertex != NULL);
    assert(vertex->position() != destination);
    
    if (point())
        return movePointVertex(vertex, destination);
    else if (edge())
        return moveEdgeVertex(vertex, destination, allowMergeIncidentVertex);
    else if (polygon())
        return movePolygonVertex(vertex, destination, allowMergeIncidentVertex);
    else
        return movePolyhedronVertex(vertex, destination, allowMergeIncidentVertex, callback);
}

template <typename T, typename FP>
typename Polyhedron<T,FP>::MoveVertexResult Polyhedron<T,FP>::movePointVertex(Vertex* vertex, const V& destination) {
    const V originalPosition(vertex->position());
    vertex->setPosition(destination);
    return MoveVertexResult(MoveVertexResult::Type_VertexMoved, originalPosition, vertex);
}

template <typename T, typename FP>
typename Polyhedron<T,FP>::MoveVertexResult Polyhedron<T,FP>::moveEdgeVertex(Vertex* vertex, const V& destination, const bool allowMergeIncidentVertex) {
    const V originalPosition(vertex->position());
    Edge* edge = *m_edges.begin();
    Vertex* other = edge->otherVertex(vertex);
    if (other->position() == destination) {
        if (!allowMergeIncidentVertex)
            return MoveVertexResult(MoveVertexResult::Type_VertexUnchanged, originalPosition, vertex);
        
        m_edges.remove(edge);
        delete edge;
        m_vertices.remove(vertex);
        delete vertex;
        
        return MoveVertexResult(MoveVertexResult::Type_VertexMoved, originalPosition, other);
    } else {
        return movePointVertex(vertex, destination);
    }
}

template <typename T, typename FP>
typename Polyhedron<T,FP>::MoveVertexResult Polyhedron<T,FP>::movePolygonVertex(Vertex* vertex, const V& destination, const bool allowMergeIncidentVertex) {
    const V originalPosition(vertex->position());
    Face* face = *m_faces.begin();
    if (face->pointStatus(destination) != Math::PointStatus::PSInside)
        return MoveVertexResult(MoveVertexResult::Type_VertexUnchanged, originalPosition, vertex);
    
    Vertex* occupant = findVertexByPosition(destination);
    if (occupant != NULL) {
        HalfEdge* connectingEdge = vertex->findConnectingEdge(occupant);
        if (connectingEdge == NULL)
            connectingEdge = occupant->findConnectingEdge(vertex);
        if (!allowMergeIncidentVertex || connectingEdge == NULL)
            return MoveVertexResult(MoveVertexResult::Type_VertexUnchanged, originalPosition, vertex);

        Vertex* origin = connectingEdge->origin();
        Vertex* destVertex = connectingEdge->destination();
        
        Edge* edge = connectingEdge->edge();
        face->removeFromBoundary(connectingEdge);
        
        m_edges.remove(edge);
        delete edge;
        delete connectingEdge;
        m_vertices.remove(origin);
        delete origin;
        
        return MoveVertexResult(MoveVertexResult::Type_VertexMoved, originalPosition, destVertex);
    } else {
        return movePointVertex(vertex, destination);
    }
}

template <typename T, typename FP> template <typename C>
typename Polyhedron<T,FP>::MoveVertexResult Polyhedron<T,FP>::movePolyhedronVertex(Vertex* vertex, const V& destination, const bool allowMergeIncidentVertex, C& callback) {
    // The idea of this algorithm can be summarized as follows:
    // First, break up all faces incident to the given vertex so that they become triangles.
    // Second, examine the (straight) path along which the given vertex will travel when moved to the
    // given location. Determine the closest point on this path where one of the incident triangles
    // becomes coplanar with one (or more) of its neighbours. This point is given by the value of
    // curFrac, which is between 0.0 and 1.0, 0.0 being the original vertex position and 1.0 the destination.
    // Third, determine whether the vertex will come to rest on another vertex. If that is the case, and
    // allowMergeIncidentVertices is true, then merge those vertices if possible and conclude the operation.
    // Otherwise, proceed with the fourth step: Move the vertex to the point we just computed, and merge the
    // incident faces with their neighbours.
    // Continue this until the vertex is either deleted during the merging phase, or until the vertex arrives
    // at its destination position.
    
    T lastFrac = 0.0;
    const V originalPosition = vertex->position();
    while (!vertex->position().equals(destination, 0.0)) {
        splitIncidentFaces(vertex, destination, callback);
        
        const T curFrac = computeNextMergePoint(vertex, originalPosition, destination, lastFrac);
        if (curFrac < 0.0)
            return MoveVertexResult(MoveVertexResult::Type_VertexUnchanged, originalPosition, vertex);
        
        assert(curFrac > lastFrac);
        lastFrac = curFrac;
        
        const V newPosition = originalPosition + lastFrac * (destination - originalPosition);
        Vertex* occupant = findVertexByPosition(newPosition);
        if (occupant != NULL) {
            HalfEdge* connectingEdge = vertex->findConnectingEdge(occupant);
            if (!allowMergeIncidentVertex || connectingEdge == NULL) {
                mergeIncidentFaces(vertex, callback);
                const MoveVertexResult result = moveVertex(vertex, originalPosition, false, callback);
                _UNUSED(result);
                assert(result.moved());
                return MoveVertexResult(MoveVertexResult::Type_VertexUnchanged, originalPosition, vertex);
            }
            mergeVertices(connectingEdge, callback);
        }
        
        vertex->setPosition(newPosition);
        vertex = cleanupAfterVertexMove(vertex, callback);
        if (vertex == NULL)
            return MoveVertexResult(MoveVertexResult::Type_VertexDeleted, originalPosition);
    }
    
    return MoveVertexResult(MoveVertexResult::Type_VertexMoved, originalPosition, vertex);
}

// Splits the faces incident to the given vertex so that the polyhedron cannot become
// convex if the vertex is moved to the given destination. Depending on the orientation of
// an incident face and the vector from the given vertex's position to the destination,
// this includes chopping off triangles of some incident faces, and splitting other faces
// into triangle fans.
template <typename T, typename FP> template <typename C>
void Polyhedron<T,FP>::splitIncidentFaces(Vertex* vertex, const V& destination, C& callback) {
    HalfEdge* firstEdge = vertex->leaving();
    HalfEdge* curEdge = firstEdge;
    
    do {
        HalfEdge* nextEdge = curEdge->nextIncident();
        Face* face = curEdge->face();
        if (face->vertexCount() > 3) {
            const Math::PointStatus::Type status = face->pointStatus(destination);
            if (status == Math::PointStatus::PSBelow)
                chopFace(face, curEdge, callback);
            else
                splitFace(face, curEdge, callback);
        }
        curEdge = nextEdge;
    } while (curEdge != firstEdge);
}

// Creates a new face by chopping off one triangle of the given face. The triangle
// will have the destination of the given edge, the origin of the given edge, and
// the origin of the given edge's predecessor as its vertices.
template <typename T, typename FP> template <typename C>
void Polyhedron<T,FP>::chopFace(Face* face, HalfEdge* halfEdge, C& callback) {
    assert(face->vertexCount() > 3);
    
    HalfEdge* next = halfEdge;
    HalfEdge* previous = next->previous();
    
    HalfEdge* newEdge1 = new HalfEdge(previous->origin());
    HalfEdge* newEdge2 = new HalfEdge(next->destination());
    
    face->replaceBoundary(previous, next, newEdge1);
    
    assert(next->next() == previous);
    assert(previous->previous() == next);
    
    HalfEdgeList boundary;
    boundary.append(previous, 2);
    boundary.append(newEdge2, 1);
    
    Face* newFace = new Face(boundary);
    callback.faceWasSplit(face, newFace);
    
    m_faces.append(newFace, 1);
    m_edges.append(new Edge(newEdge1, newEdge2), 1);
}

/*
 Splits the given face into triangles by adding new edges from the origin of the given edge
 to every other non-adjacent vertex in the given face.
 ______     ______
 |    |     |   /|
 |    |     |  / |
 |    |     | /  |
 |    |     |/   |
 --h-->     --h-->
 
 */
template <typename T, typename FP> template <typename C>
void Polyhedron<T,FP>::splitFace(Face* face, HalfEdge* halfEdge, C& callback) {
    HalfEdge* next = halfEdge;
    while (face->vertexCount() > 3) {
        HalfEdge* previous = next->previous();
        chopFace(face, previous, callback);
    }
}

template <typename T, typename FP>
T Polyhedron<T,FP>::computeNextMergePoint(Vertex* vertex, const V& origin, const V& destination, T lastFrac) const {
    HalfEdge* firstEdge = vertex->leaving();
    HalfEdge* curEdge = firstEdge;
    
    T minFrac = 1.0;
    do {
        minFrac = std::min(minFrac, computeNextMergePointForIncidentNeighbour(curEdge, origin, destination, lastFrac));
        minFrac = std::min(minFrac, computeNextMergePointForOppositeNeighbour(curEdge, origin, destination, lastFrac));
        curEdge = curEdge->nextIncident();
    } while (curEdge != firstEdge);
    return minFrac;
}

template <typename T, typename FP>
T Polyhedron<T,FP>::computeNextMergePointForIncidentNeighbour(HalfEdge* edge, const V& origin, const V& destination, T lastFrac) const {
    /*
     We consider the plane made up by the points p1, p2 and p3 of side and next. If the movement
     of the vertex were to go through this plane, the brush would become convex, which we must prevent.
     
     v----p3
     |\ n |
     | \  |
     |  \ |
     | s \|
     p1---p2
     
     */
    
    HalfEdge* nextEdge = edge->nextIncident();
    HalfEdge* nextNextEdge = nextEdge->nextIncident();
    
    assert(edge->face()->vertexCount() == 3);
    assert(nextEdge->face()->vertexCount() == 3);
    
    Vertex* v1 = edge->destination();
    Vertex* v2 = nextEdge->destination();
    Vertex* v3 = nextNextEdge->destination();
    
    const V& p1 = v1->position();
    const V& p2 = v2->position();
    const V& p3 = v3->position();
    
    Plane<T,3> plane;
    if (!setPlanePoints(plane, p1, p2, p3)) {
        // The points are colinear and we cannot determine the move distance - this is an error!
        return -1.0;
    }
    
    return computeNextMergePointForPlane(origin, destination, plane, lastFrac);
}

template <typename T, typename FP>
T Polyhedron<T,FP>::computeNextMergePointForOppositeNeighbour(HalfEdge* edge, const V& origin, const V& destination, T lastFrac) const {
    /*
     We consider the boundary plane of the one neighbour to the side which is not incident to the
     moved vertex. This neighbour is not necessarily a triangle, but that does not matter.
     
             p3------
            /    n  |
           /     e  |
     v----p2     i  |
     |\ s |      g  |
     | \  |      h  |
     |  \ |      b  |
     |   \|      o  |
     -----p1     u  |
           \     r  |
            ---------
     */
    
    HalfEdge* myBorder = edge->next();
    HalfEdge* theirBorder = myBorder->twin();
    
    Vertex* v1 = edge->destination();
    Vertex* v2 = theirBorder->origin();
    Vertex* v3 = theirBorder->previous()->origin();
    
    const V& p1 = v1->position();
    const V& p2 = v2->position();
    const V& p3 = v3->position();
    
    Plane<T,3> plane;
    if (!setPlanePoints(plane, p1, p2, p3)) {
        // The points are colinear and we cannot determine the move distance - this is an error!
        return -1.0;
    }
    
    return computeNextMergePointForPlane(origin, destination, plane, lastFrac);
}

template <typename T, typename FP>
T Polyhedron<T,FP>::computeNextMergePointForPlane(const V& origin, const V& destination, const Plane<T,3>& plane, T lastFrac) const {
    const T origDot = origin.dot(plane.normal) - plane.distance;
    const T destDot = destination.dot(plane.normal) - plane.distance;
    
    // We only consider this a merge point if the origin does not lie (almost) within the plane.
    if (std::abs(origDot) >= 0.001) {
        // Does the vertex travel through the plane?
        if ((origDot > 0.0) != (destDot > 0.0)) {
            // The distance must grow because we use the original vertex origin.
            const T frac = std::abs(origDot) / (std::abs(origDot) + std::abs(destDot));
            if (frac > lastFrac)
                return frac;
        }
    }
    return 1.0;
}

// Merges the origin and destination vertex of the given edge into one vertex, thereby
// deleting the edge, the destination vertex, and the faces incident to the connecting edge
// and its twin.
// Assumes that these incident faces are triangles.
template <typename T, typename FP> template <typename C>
void Polyhedron<T,FP>::mergeVertices(HalfEdge* connectingEdge, C& callback) {
    HalfEdge* oppositeEdge = connectingEdge->twin();
    
    Vertex* origin = connectingEdge->origin();
    Vertex* destination = oppositeEdge->origin();
    
    // First we merge the triangles that will become invalid by the merge to their neighbours.
    // We assume they are both triangles.
    assert(connectingEdge->face()->vertexCount() == 3);
    assert(oppositeEdge->face()->vertexCount() == 3);
    mergeNeighbours(connectingEdge->previous(), callback);
    mergeNeighbours(oppositeEdge->next(), callback);
    
    // Now we delete the destination of the connecting edge.
    // First we have to change the origin of all edges originating
    // at the destination to the origin of the connecting edge.
    // We also have to delete the connecting edge and its twin from the incident faces.
    
    destination->setLeaving(connectingEdge->next());
    
    HalfEdge* firstEdge = destination->leaving();
    HalfEdge* curEdge = firstEdge;
    do {
        HalfEdge* next = curEdge->nextIncident();
        curEdge->setOrigin(origin);
        curEdge = next;
    } while (curEdge != firstEdge);
    
    Face* leftFace = connectingEdge->face();
    leftFace->removeFromBoundary(connectingEdge);
    
    Face* rightFace = oppositeEdge->face();
    rightFace->removeFromBoundary(oppositeEdge);
    
    Edge* edge = connectingEdge->edge();
    m_edges.remove(edge);
    delete edge;
    
    delete connectingEdge;
    delete oppositeEdge;
    
    m_vertices.remove(destination);
    delete destination;
}

// Merges all leaving edges of the given vertex with their successors if they have become colinear.
// Also merges all faces incident to the given vertex with their coplanar neighbours.
// Finally merges all arriving edges of the given vertex with any colinear leaving edges.
// Returns the given vertex or NULL if it was deleted.
template <typename T, typename FP> template <typename C>
typename Polyhedron<T,FP>::Vertex* Polyhedron<T,FP>::cleanupAfterVertexMove(Vertex* vertex, C& callback) {
    mergeLeavingEdges(vertex, callback);
    vertex = mergeIncidentFaces(vertex, callback);
    if (vertex != NULL)
        vertex = mergeIncomingAndLeavingEdges(vertex, callback);
    assert(checkNoCoplanarFaces());
    return vertex;
}

// Merges all leaving edges of the given vertex with their successors if they have become colinear.
template <typename T, typename FP> template <typename C>
void Polyhedron<T,FP>::mergeLeavingEdges(Vertex* vertex, C& callback) {
    HalfEdge* curEdge = vertex->leaving();
    do {
        Vertex* destination = curEdge->destination();
        HalfEdge* colinearEdge = destination->findColinearEdge(curEdge);
        if (colinearEdge != NULL) {
            mergeNeighboursOfColinearEdges(curEdge, colinearEdge, callback);
            mergeColinearEdges(curEdge, colinearEdge);
        }
        curEdge = curEdge->nextIncident();
    } while (curEdge != vertex->leaving());
}

// Merges all arriving edges of the given vertex with any colinear leaving edges.
// Returns the given vertex or NULL if it was deleted.
template <typename T, typename FP> template <typename C>
typename Polyhedron<T,FP>::Vertex* Polyhedron<T,FP>::mergeIncomingAndLeavingEdges(Vertex* vertex, C& callback) {
    HalfEdge* arriving = vertex->leaving()->twin();
    do {
        HalfEdge* colinearLeaving = vertex->findColinearEdge(arriving);
        if (colinearLeaving != NULL) {
            mergeNeighboursOfColinearEdges(arriving, colinearLeaving, callback);
            mergeColinearEdges(arriving, colinearLeaving);
            return NULL;
        }
        arriving = arriving->next()->twin();
    } while (arriving != vertex->leaving()->twin());
    return vertex;
}

// Merges the neighbours of the given successive colinear edges with their coplanar neighbours.
template <typename T, typename FP> template <typename C>
void Polyhedron<T,FP>::mergeNeighboursOfColinearEdges(HalfEdge* edge1, HalfEdge* edge2, C& callback) {
    assert(edge1->destination() == edge2->origin());
    
    if (edge1->face()->vertexCount() == 3 && edge1->next() == edge2) // the right side is a degenerate triangle now
        mergeNeighbours(edge1->previous(), callback);
    else if (edge1->next()->face() != edge2->face()) // the face might already have been merged previously
        mergeNeighbours(edge1->next(), callback);
    
    if (edge1->twin()->face()->vertexCount() == 3 && edge1->twin()->previous() == edge2->twin())
        mergeNeighbours(edge1->twin()->next(), callback);
    else if (edge1->twin()->face() != edge2->twin()->face())
        mergeNeighbours(edge1->twin()->previous(), callback);
}

// Merges the given successive colinear edges. As a result, the origin of the second
// given edge, the second given edge itself and its twin are removed. At the same time,
// the first given edge replaces the second given edge, and the first given edge's twin
// replaces the twin of the second given edge.
// Assumes that the incident faces have already been merged.
template <typename T, typename FP>
void Polyhedron<T,FP>::mergeColinearEdges(HalfEdge* edge1, HalfEdge* edge2) {
    assert(edge1->destination() == edge2->origin());
    assert(edge1->face() == edge2->face());
    assert(edge1->twin()->face() == edge2->twin()->face());
    
    Vertex* vertex = edge2->origin();
    Vertex* succ = edge2->destination();
    
    edge1->twin()->setOrigin(succ);
    succ->setLeaving(edge1->twin());
    
    HalfEdge* toRemove1 = edge2;
    HalfEdge* toRemove2 = toRemove1->twin();
    
    Face* left = toRemove1->face();
    left->removeFromBoundary(toRemove1);
    
    Face* right = toRemove2->face();
    right->removeFromBoundary(toRemove2);
    
    Edge* edge = toRemove1->edge();
    m_edges.remove(edge);
    delete edge;
    
    delete toRemove1;
    delete toRemove2;
    
    m_vertices.remove(vertex);
    delete vertex;
}

// Merges all faces incident to the given vertex with their coplanar neighbours.
// Returns the given vertex or NULL if the given vertex was deleted.
template <typename T, typename FP> template <typename C>
typename Polyhedron<T,FP>::Vertex* Polyhedron<T,FP>::mergeIncidentFaces(Vertex* vertex, C& callback) {
    size_t incidentFaceCount = 0;
    bool allCoplanar = true;
    
    // First determine whether all incident faces have become coplanar. While doing this,
    // also remember an edge such that its incident face and its inner neighbour are not
    // coplanar.
    HalfEdge* firstEdge = vertex->leaving();
    HalfEdge* curEdge = firstEdge;
    do {
        Face* face = curEdge->face();
        HalfEdge* innerBorder = curEdge->previous();
        Face* innerNeighbour = innerBorder->twin()->face();
        
        if (!face->coplanar(innerNeighbour)) {
            allCoplanar = false;
            firstEdge = curEdge;
        }
        curEdge = innerBorder->twin();
        ++incidentFaceCount;
    } while (allCoplanar && curEdge != firstEdge);
    
    if (allCoplanar) {
        HalfEdgeList boundary;
        
        // Now we iterate using the incident face count because we can't rely
        // on the curEdge's twin while we're deleting the edges we encounter.
        curEdge = firstEdge;
        Face* remaining = curEdge->face();
        
        for (size_t i = 0; i < incidentFaceCount; ++i) {
            Face* face = curEdge->face();
            Edge* edge = curEdge->edge();
            
            HalfEdge* twin = curEdge->twin();
            HalfEdge* outerBorder = curEdge->next();
            
            // Don't increment past the last edge because its twin will already have been deleted.
            if (i < incidentFaceCount - 1)
                curEdge = curEdge->nextIncident();
            
            twin->origin()->setLeaving(outerBorder);
            
            face->removeFromBoundary(outerBorder);
            boundary.append(outerBorder, 1);
            
            if (i > 0) { // We want to keep the original face around
                callback.facesWillBeMerged(remaining, face);
                m_faces.remove(face);
                delete face;
            }

            m_edges.remove(edge);
            delete edge;
        }
        
        HalfEdgeList oldBoundary = remaining->replaceEntireBoundary(boundary);
        oldBoundary.deleteAll();
        
        m_vertices.remove(vertex);
        delete vertex;
        vertex = NULL;
    } else {
        // Due to how we have chosen the first edge, we know that the first two
        // incident faces are not coplanar and thus won't be merged. This is important
        // because otherwise, the loop would immediately terminate after one iteration
        // since curEdge is not incremented when two inner neighbours are merged.
        curEdge = firstEdge;
        do {
            Face* face = curEdge->face();
            
            HalfEdge* outerBorder = curEdge->next();
            Face* outerNeighbour = outerBorder->twin()->face();
            
            HalfEdge* innerBorder = curEdge->previous();
            Face* innerNeighbour = innerBorder->twin()->face();
            
            if (face->coplanar(innerNeighbour)) {
                // Ensure that we don't remove the first edge, otherwise we'll loop endlessly.
                if (innerBorder->twin() == firstEdge)
                    firstEdge = firstEdge->nextIncident();
                mergeNeighbours(innerBorder, callback);
            } else if (face->coplanar(outerNeighbour)) {
                mergeNeighbours(outerBorder, callback);
                curEdge = curEdge->nextIncident();
            } else {
                curEdge = curEdge->nextIncident();
            }
        } while (curEdge != firstEdge);
    }
    return vertex;
}

// Merges the face incident to the given edge (called border) with its neighbour, i.e.
// the face incident to the given face's twin. The face incident to the border is deleted
// while the neighbour consumes the boundary of the incident face.
// Also handles the case where the border is longer than just one edge.
template <typename T, typename FP> template <typename C>
void Polyhedron<T,FP>::mergeNeighbours(HalfEdge* borderFirst, C& callback) {
    Face* face = borderFirst->face();
    Face* neighbour = borderFirst->twin()->face();
    
    while (borderFirst->previous()->face() == face &&
           borderFirst->previous()->twin()->face() == neighbour) {
        borderFirst = borderFirst->previous();
    }
    
    HalfEdge* twinLast = borderFirst->twin();
    HalfEdge* borderLast = borderFirst;
    
    while (borderLast->next()->face() == face &&
           borderLast->next()->twin()->face() == neighbour) {
        borderLast = borderLast->next();
    }
    
    HalfEdge* twinFirst = borderLast->twin();
    
    borderFirst->origin()->setLeaving(twinLast->next());
    twinFirst->origin()->setLeaving(borderLast->next());
    
    HalfEdge* remainingFirst = borderLast->next();
    HalfEdge* remainingLast = borderFirst->previous();
    
    face->removeFromBoundary(borderFirst, borderLast);
    face->removeFromBoundary(remainingFirst, remainingLast);
    
    neighbour->replaceBoundary(twinFirst, twinLast, remainingFirst);
    
    HalfEdge* cur = borderFirst;
    do {
        Edge* edge = cur->edge();
        HalfEdge* next = cur->next();
        HalfEdge* twin = cur->twin();
        Vertex* origin = cur->origin();
        
        m_edges.remove(edge);
        delete edge;
        
        delete cur;
        delete twin;

        if (cur != borderFirst) {
            m_vertices.remove(origin);
            delete origin;
        }
        
        cur = next;
    } while (cur != borderFirst);
    
    callback.facesWillBeMerged(neighbour, face);
    m_faces.remove(face);
    delete face;
}

#endif
