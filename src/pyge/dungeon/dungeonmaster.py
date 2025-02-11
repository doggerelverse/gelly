#=================================================================================================
# Dungeon Master Class
#
#
#=================================================================================================

#from dungeon import Dungeon

#from character import Character

from character.action import Action
from battlemaster import BattleMaster

#=================================================================================================
#TODO#
#1. list of characters should be a priority queue sorted in turn order
#=================================================================================================
class DungeonMaster(object):
  """
  """
  def __init__(self,dungeon):
    """
    """
    print "Initializing DM"

    self.dungeon = dungeon
    self.battlemaster = BattleMaster( )
    #=============================================================================================
    self._characters = list( )
    for each in dungeon.characters.values( ):
      self._characters.append( each )
    self._current = 0
    self._effects = []
    #=============================================================================================
  #===============================================================================================
  def resolve_action( self, action ):
    """ """
    if action.type == 'attack':
      self.battlemaster.resolve_combat( action )
#      self.update()
      return action.skill.effect
    elif action.type == 'move':
      self.resolvemove( action )
  #===============================================================================================
  def resolvemove( self, moveaction ):
    """
    """
    moveaction.stage.movecharacter( moveaction.actor, moveaction.target )
  #===============================================================================================
  def resume( self ):
    """
    """
    self._hold = False
  #===============================================================================================
  def turn( self ):
    """
      calculate a full game turn
    """
    effects = []
    while True:

      if self._current >= len(self._characters): #circular increment
        self._current = 0

      char = self._characters[self._current] 

      #EMIT EVENT TURN START
      #EVENT System
      char.listen()
      if not char._alive: 
        self.update()
        return

      action = char.mind.getaction() #request an action from the current character
      if action: #npc's will always return an action, might have to wait on pc's
        effect = self.resolve_action( action )
        if effect:
          self._effects.append( effect )
      else: #pc's will return false if no action is in the queue
        break
       # return effects#in this case, we return, waiting for character to perform an action
      self._current += 1 #move to the next character




#ugly hackishness, placing each effect at the coordinates of the current characters target
    print self._effects
    for e in self._effects:
      if char.mind._target:
        loc = self.dungeon.loc( char.mind._target )
      else:
        loc = self.dungeon.loc( char )
        loc = (loc[0],loc[1]+1)
      self.dungeon.place( e, loc )
  #===============================================================================================
  def update( self ):
    """
      update the internal state

      handles checks for dead characters/objects
    """
    for character in self._characters:
      if character.health <= 0:
        character._alive = False
    self.prune()
    for i,e in enumerate(self._effects):
      if not e.alive:
        self.dungeon.remove(e)  #remove the effect from the dungeon
        self._effects.pop(i)    #we no longer need to track this effect
        e.reset()   #reset the effect, so it's ready for it's next use
  #===============================================================================================
  def prune( self ):
    """
      Remove (prune) all dead objects from the internal lists
    """
    prune = [character for character in self._characters if not character._alive]
    self._characters = [character for character in self._characters if character not in prune]
    for character in prune:
      self.dungeon.remove( character )
  #===============================================================================================
  def foo( self ):
    """
      
    """
    for effect,pos in effects:
      self.dungeon.place( effect, pos )
#=================================================================================================
